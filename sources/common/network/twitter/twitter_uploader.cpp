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

#include "oauth/http_request.hpp"

#include "twitter_uploader.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  TwitterUploader::TwitterUploader (Listener *listener)
  {
    _request     = NULL;
    _http_header = NULL;
    _postfields  = NULL;

    _listener = listener;
    if (_listener)
    {
      _listener->Use ();
    }
  }

  // --------------------------------------------------------------------------------
  TwitterUploader::~TwitterUploader ()
  {
    Object::TryToRelease (_request);

    if (_http_header)
    {
      curl_slist_free_all (_http_header);
    }

    g_free (_postfields);

    if (_listener)
    {
      _listener->Drop ();
    }
  }

  // --------------------------------------------------------------------------------
  void TwitterUploader::UpLoadRequest (Oauth::HttpRequest *request)
  {
    if (_request == NULL)
    {
      Retain ();

      _request = request;
      _request->Retain ();

      {
        GError  *error         = NULL;
        GThread *sender_thread;

        sender_thread = g_thread_try_new ("TwitterUploader",
                                          (GThreadFunc) ThreadFunction,
                                          this,
                                          &error);
        if (sender_thread == NULL)
        {
          g_printerr ("Failed to create Uploader thread: %s\n", error->message);
          g_error_free (error);
          _request->Release ();
          _request = NULL;
          Release ();
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  size_t TwitterUploader::OnResponseHeader (char            *buffer,
                                            size_t           size,
                                            size_t           nitems,
                                            TwitterUploader *uploader)
  {
    guint  load            = size *nitems;
    gchar *header_response = (gchar *) g_malloc0 (load + 1);

    memcpy (header_response,
            buffer,
            load);

    if (g_strrstr (header_response, "x-rate-limit-limit:"))
    {
      gchar **fields = g_strsplit (header_response,
                                   ":",
                                   -1);
      if (fields[1])
      {
        uploader->_request->SetRateLimitLimit (g_ascii_strtoull (fields[1],
                                                                 NULL,
                                                                 10));
      }
      g_strfreev (fields);
    }
    else if (g_strrstr (header_response, "x-rate-limit-remaining:"))
    {
      gchar **fields = g_strsplit (header_response,
                                   ":",
                                   -1);
      if (fields[1])
      {
        uploader->_request->SetRateLimitRemaining (g_ascii_strtoull (fields[1],
                                                                     NULL,
                                                                     10));
      }
      g_strfreev (fields);
    }
    else if (g_strrstr (header_response, "x-rate-limit-reset:"))
    {
      gchar **fields = g_strsplit (header_response,
                                   ":",
                                   -1);
      if (fields[1])
      {
        uploader->_request->SetRateLimitReset (g_ascii_strtoull (fields[1],
                                                                 NULL,
                                                                 10));
      }
      g_strfreev (fields);
    }

    g_free (header_response);

    return load;
  }

  // --------------------------------------------------------------------------------
  void TwitterUploader::SetCurlOptions (CURL *curl)
  {
    Uploader::SetCurlOptions (curl);

    curl_easy_setopt (curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt (curl, CURLOPT_HEADERFUNCTION, OnResponseHeader);
    curl_easy_setopt (curl, CURLOPT_HEADERDATA,     this);

    if (g_strcmp0 (_request->GetMethod (), "POST") == 0)
    {
      curl_easy_setopt (curl, CURLOPT_POST, 1L);

      g_free (_postfields);
      _postfields = _request->GetParameters ();
      if (_postfields)
      {
        curl_easy_setopt (curl, CURLOPT_POSTFIELDS, _postfields);
      }
    }
    else
    {
      curl_easy_setopt (curl, CURLOPT_POST, 0L);
    }

    if (_http_header)
    {
      curl_slist_free_all (_http_header);
      _http_header = NULL;
    }

    {
      gchar *header = _request->GetHeader ();

      _http_header = curl_slist_append (_http_header, header);
      curl_easy_setopt (curl, CURLOPT_HTTPHEADER, _http_header);

      g_free (header);
    }
  }

  // --------------------------------------------------------------------------------
  const gchar *TwitterUploader::GetUrl ()
  {
    return _request->GetURL ();
  }

  // --------------------------------------------------------------------------------
  gboolean TwitterUploader::OnThreadDone (TwitterUploader *uploader)
  {
    uploader->_listener->OnTwitterResponse (uploader->_request);
    uploader->Release ();

    return G_SOURCE_REMOVE;
  }

  // --------------------------------------------------------------------------------
  void TwitterUploader::OnUploadDone (const gchar *response)
  {
    _request->ParseResponse (response);
  }

  // --------------------------------------------------------------------------------
  gpointer TwitterUploader::ThreadFunction (TwitterUploader *uploader)
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

    return NULL;
  }
}
