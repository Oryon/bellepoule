// Copyright (C) 2011 Yannick Le Roux.
// This file is part of BellePoule.
//
//   BellePoule is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   BellePoule is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with BellePoule.  If not, see <http://www.gnu.org/licenses/>.

#include <string.h>

#include "util/global.hpp"
#include "v1_request.hpp"
#include "uploader.hpp"

namespace Oauth
{
  // --------------------------------------------------------------------------------
  Uploader::Uploader (Listener *listener)
  {
    _request        = nullptr;
    _http_header    = nullptr;
    _url_parameters = nullptr;

    _response_header = g_hash_table_new_full (g_str_hash,
                                              g_str_equal,
                                              g_free,
                                              g_free);

    _listener = listener;
    if (_listener)
    {
      _listener->Use ();
    }
  }

  // --------------------------------------------------------------------------------
  Uploader::~Uploader ()
  {
    Object::TryToRelease (_request);

    if (_http_header)
    {
      curl_slist_free_all (_http_header);
    }

    g_free (_url_parameters);

    if (_listener)
    {
      _listener->Drop ();
    }

    g_hash_table_destroy (_response_header);
  }

  // --------------------------------------------------------------------------------
  void Uploader::UpLoadRequest (Request *request)
  {
    if (_request == nullptr)
    {
      Retain ();

      _request = request;
      _request->Retain ();

      {
        GError  *error         = nullptr;
        GThread *sender_thread;

        sender_thread = g_thread_try_new ("Oauth::Uploader",
                                          (GThreadFunc) ThreadFunction,
                                          this,
                                          &error);
        if (sender_thread == nullptr)
        {
          g_printerr ("Failed to create Uploader thread: %s\n", error->message);
          g_error_free (error);
          _request->Release ();
          _request = nullptr;
          Release ();
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  size_t Uploader::OnResponseHeader (char     *buffer,
                                     size_t    size,
                                     size_t    nitems,
                                     Uploader *uploader)
  {
    guint  load   = size * nitems;
    gchar *header = (gchar *) g_malloc0 (load + 1);

    memcpy (header,
            buffer,
            load);

    {
      gchar **fields = g_strsplit (header,
                                   ":",
                                   -1);
      if (fields[0] && fields[1])
      {
        g_hash_table_replace (uploader->_response_header,
                              g_strdup (fields[0]),
                              g_strdup (fields[1]));
      }
      g_strfreev (fields);
    }

    g_free (header);

    return load;
  }

  // --------------------------------------------------------------------------------
  void Uploader::SetCurlOptions (CURL *curl)
  {
    Net::Uploader::SetCurlOptions (curl);

    curl_easy_setopt (curl, CURLOPT_SSL_VERIFYPEER, 1L);

    {
      gchar *ca_bundle = g_build_filename (Global::_share_dir, "resources", "cacert.pem", NULL);

      if (g_file_test (ca_bundle, G_FILE_TEST_EXISTS))
      {
        curl_easy_setopt (curl, CURLOPT_CAINFO, ca_bundle);
      }
      g_free (ca_bundle);
    }

    curl_easy_setopt (curl, CURLOPT_HEADERFUNCTION, OnResponseHeader);
    curl_easy_setopt (curl, CURLOPT_HEADERDATA,     this);

    g_free (_url_parameters);
    _url_parameters = _request->GetParameters ();

    if (g_strcmp0 (_request->GetMethod (), "POST") == 0)
    {
      curl_easy_setopt (curl, CURLOPT_POST, 1L);

      if (_url_parameters)
      {
        curl_easy_setopt (curl, CURLOPT_POSTFIELDS, _url_parameters);
      }
    }
    else
    {
      curl_easy_setopt (curl, CURLOPT_POST, 0L);

      if (_url_parameters)
      {
        gchar *extended_url = g_strdup_printf ("%s?%s", GetUrl (), _request->GetParameters ());

        curl_easy_setopt (curl, CURLOPT_URL, extended_url);
        g_free (extended_url);
      }
    }

    if (g_strcmp0 (_request->GetMethod (), "DELETE") == 0)
    {
      curl_easy_setopt (curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    }

    if (_http_header)
    {
      curl_slist_free_all (_http_header);
    }

    _http_header = _request->GetHeader ();
    if (_http_header)
    {
      curl_easy_setopt (curl, CURLOPT_HTTPHEADER, _http_header);
    }
  }

  // --------------------------------------------------------------------------------
  const gchar *Uploader::GetUrl ()
  {
    return _request->GetURL ();
  }

  // --------------------------------------------------------------------------------
  gboolean Uploader::OnThreadDone (Uploader *uploader)
  {
    uploader->_request->ParseResponse (uploader->_response_header,
                                       uploader->GetResponseBody ());
    uploader->_listener->OnServerResponse (uploader->_request);
    uploader->Release ();

    return G_SOURCE_REMOVE;
  }

  // --------------------------------------------------------------------------------
  gpointer Uploader::ThreadFunction (Uploader *uploader)
  {
    CURLcode curl_code = uploader->Upload ();

    if (curl_code != CURLE_OK)
    {
      g_print (RED "[Uploader Error] " ESC "%s\n", curl_easy_strerror (curl_code));
    }
    else
    {
#ifdef UPLOADER_DEBUG
      g_print (YELLOW "[Uploader] " ESC "Done\n");
#endif
    }

    g_idle_add ((GSourceFunc) OnThreadDone,
                uploader);

    return nullptr;
  }
}
