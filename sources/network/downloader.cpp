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
                          Callback     callback,
                          gpointer     user_data)
    : Object ("Downloader")
  {
    _callback = callback;
    _callback_data._downloader = this;
    _callback_data._user_data  = user_data;

    _data    = NULL;
    _address = NULL;

    _killed = FALSE;

    _name = g_strdup (name);
  }

  // --------------------------------------------------------------------------------
  Downloader::~Downloader ()
  {
    g_free (_data);
    g_free (_address);
    g_free (_name);
  }

  // --------------------------------------------------------------------------------
  void Downloader::Start (const gchar *address,
                          guint        refresh_period)
  {
    GError *error = NULL;

    _address = g_strdup (address);
    _period  = refresh_period;

    Retain ();

    if (!g_thread_create ((GThreadFunc) ThreadFunction,
                          this,
                          FALSE,
                          &error))
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
    while (downloader->_killed == FALSE)
    {
      CURL *curl_handle;

      downloader->_data = (char *) g_malloc (1);
      downloader->_size = 0;

      curl_handle = curl_easy_init ();

      curl_easy_setopt (curl_handle,
                        CURLOPT_URL,
                        downloader->_address);

      curl_easy_setopt (curl_handle,
                        CURLOPT_WRITEFUNCTION,
                        AddText);

      curl_easy_setopt (curl_handle,
                        CURLOPT_WRITEDATA,
                        downloader);

      curl_easy_setopt (curl_handle,
                        CURLOPT_USERAGENT,
                        "libcurl-agent/1.0");

#if 0
      curl_easy_setopt (curl_handle,
                        CURLOPT_FOLLOWLOCATION,
                        1);

      curl_easy_setopt (curl_handle,
                        CURLOPT_MAXREDIRS,
                        10);
#endif

      curl_easy_perform (curl_handle);

      curl_easy_cleanup (curl_handle);

      g_idle_add ((GSourceFunc) downloader->_callback,
                  &downloader->_callback_data);

      if (downloader->_period == 0)
      {
        break;
      }

#ifdef WINDOWS_TEMPORARY_PATCH
      Sleep (downloader->_period);
#else
      sleep (downloader->_period/1000);
#endif
      g_print ("%s\n", downloader->_name);
    }

    downloader->Release ();

    return NULL;
  }

  // --------------------------------------------------------------------------------
  gchar *Downloader::GetData ()
  {
    if (_size)
    {
      return _data;
    }

    return NULL;
  }
}
