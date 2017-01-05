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

#include <unistd.h>

#include "downloader.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  Downloader::Downloader (const gchar *name,
                          Listener    *listener)
    : Object ("Downloader")
  {
    _listener = listener;
    _data     = NULL;
    _size     = 0;
    _address  = NULL;
    _killed   = FALSE;
    _name     = g_strdup (name);
  }

  // --------------------------------------------------------------------------------
  Downloader::~Downloader ()
  {
    g_free (_data);
    g_free (_address);
    g_free (_name);
  }

  // --------------------------------------------------------------------------------
  void Downloader::Start (const gchar *address)
  {
    GError *error = NULL;

    _address = g_strdup (address);

    Retain ();

    _thread = g_thread_try_new (_name,
                                (GThreadFunc) ThreadFunction,
                                this,
                                &error);
    if (_thread == NULL)
    {
      g_printerr ("Failed to create Downloader thread: %s\n", error->message);
      g_error_free (error);
      Release ();
    }
  }

  // --------------------------------------------------------------------------------
  void Downloader::Kill ()
  {
    _killed = TRUE;

    if (_thread)
    {
      g_thread_join (_thread);
    }
  }

  // --------------------------------------------------------------------------------
  size_t Downloader::AddText (void       *contents,
                              size_t      size,
                              size_t      nmemb,
                              Downloader *downloader)
  {
    size_t realsize = size * nmemb;

    downloader->_data = (char *) g_realloc (downloader->_data,
                                            downloader->_size + realsize + 1);
    if (downloader->_data)
    {
      memcpy (&(downloader->_data[downloader->_size]),
              contents,
              realsize);

      downloader->_size += realsize;
      downloader->_data[downloader->_size] = 0;
    }

    return realsize;
  }

  // --------------------------------------------------------------------------------
  gpointer Downloader::ThreadFunction (Downloader *downloader)
  {
    {
      CURL *curl_handle;

      downloader->_data = NULL;
      downloader->_size = 0;

      curl_handle = curl_easy_init ();

      curl_easy_setopt (curl_handle, CURLOPT_URL,           downloader->_address);
      curl_easy_setopt (curl_handle, CURLOPT_NOPROXY,       "127.0.0.1");
      curl_easy_setopt (curl_handle, CURLOPT_WRITEFUNCTION, AddText);
      curl_easy_setopt (curl_handle, CURLOPT_WRITEDATA,     downloader);
      curl_easy_setopt (curl_handle, CURLOPT_USERAGENT,     "libcurl-agent/1.0");

#if 0
      curl_easy_setopt (curl_handle, CURLOPT_FOLLOWLOCATION, 1);
      curl_easy_setopt (curl_handle, CURLOPT_MAXREDIRS, 10);
#endif

      curl_easy_perform (curl_handle);
      curl_easy_cleanup (curl_handle);
    }

    g_idle_add ((GSourceFunc) OnThreadDone,
                downloader);

    return NULL;
  }

  // --------------------------------------------------------------------------------
  gboolean Downloader::OnThreadDone (Downloader *downloader)
  {
    if (downloader->_killed == FALSE)
    {
      downloader->_listener->OnDownloaderData (downloader,
                                               downloader->_data);
    }

    downloader->Release ();

    return G_SOURCE_REMOVE;
  }
}
