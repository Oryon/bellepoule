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
  TwitterUploader::TwitterUploader ()
  {
    _request     = NULL;
    _http_header = NULL;
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
                                          (GThreadFunc) Loop,
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
  TwitterUploader::~TwitterUploader ()
  {
    Object::TryToRelease (_request);

    if (_http_header)
    {
      curl_slist_free_all (_http_header);
    }
  }

  // --------------------------------------------------------------------------------
  void TwitterUploader::SetCurlOptions (CURL *curl)
  {
    Uploader::SetCurlOptions (curl);

    curl_easy_setopt (curl, CURLOPT_SSL_VERIFYPEER, 1);

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
  gboolean TwitterUploader::DeferedStatus (TwitterUploader *uploader)
  {
    uploader->Release ();

    return G_SOURCE_REMOVE;
  }

  // --------------------------------------------------------------------------------
  void TwitterUploader::OnUploadDone (const gchar *response)
  {
    _request->ParseResponse (response);
  }

  // --------------------------------------------------------------------------------
  gpointer TwitterUploader::Loop (TwitterUploader *uploader)
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

    g_idle_add ((GSourceFunc) DeferedStatus,
                uploader);

    return NULL;
  }
}
