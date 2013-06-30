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

#include "uploader.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  Uploader::Uploader (const gchar *url,
                      const gchar *user,
                      const gchar *passwd)
  {
    _url    = g_strdup (url);
    _user   = g_strdup (user);
    _passwd = g_strdup (passwd);

    _full_url = NULL;
    _data     = NULL;
  }

  // --------------------------------------------------------------------------------
  Uploader::~Uploader ()
  {
    g_free (_url);
    g_free (_full_url);
    g_free (_user);
    g_free (_passwd);
    g_free (_data);
  }

  // --------------------------------------------------------------------------------
  void Uploader::UploadFile (const gchar *filename)
  {
    if (filename && _url && *_url)
    {
      gchar *base_name = g_path_get_basename (filename);

      _full_url = g_strdup_printf ("%s/%s", _url, base_name);
      g_free (base_name);
    }

    if (filename)
    {
      GError *error = NULL;

      if (g_file_get_contents (filename,
                               &_data,
                               &_data_length,
                               &error) == FALSE)
      {
        g_print ("Unable to read file: %s\n", error->message);
        g_error_free (error);
      }
    }

    Start ();
  }

  // --------------------------------------------------------------------------------
  void Uploader::UploadString (const gchar *string)
  {
    _full_url    = g_strdup (_url);
    _data        = g_strdup (string);
    _data_length = strlen (_data);

    Start ();
  }

  // --------------------------------------------------------------------------------
  void Uploader::Start ()
  {
    GError *error = NULL;

    if (!g_thread_create ((GThreadFunc) ThreadFunction,
                          this,
                          FALSE,
                          &error))
    {
      g_printerr ("Failed to create Uploader thread: %s\n", error->message);
      g_error_free (error);
      delete (this);
    }
  }

  // --------------------------------------------------------------------------------
  gpointer Uploader::ThreadFunction (Uploader *uploader)
  {
    if (uploader->_data && uploader->_full_url)
    {
      CURL *curl = curl_easy_init ();

      if (curl)
      {
        curl_easy_setopt (curl, CURLOPT_READFUNCTION,     ReadCallback);
        curl_easy_setopt (curl, CURLOPT_READDATA,         uploader);
        curl_easy_setopt (curl, CURLOPT_UPLOAD,           1L);
        curl_easy_setopt (curl, CURLOPT_URL,              uploader->_full_url);
        curl_easy_setopt (curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t) strlen (uploader->_data) + 1);
#ifdef DEBUG
        curl_easy_setopt (curl, CURLOPT_DEBUGFUNCTION,    OnUpLoadTrace);
        curl_easy_setopt (curl, CURLOPT_DEBUGDATA,        uploader);
        curl_easy_setopt (curl, CURLOPT_VERBOSE,          1L);
#endif

        if (uploader->_user && *uploader->_user && uploader->_passwd && *uploader->_passwd)
        {
          gchar *opt = g_strdup_printf ("%s:%s", uploader->_user, uploader->_passwd);

          curl_easy_setopt (curl, CURLOPT_USERPWD, opt);
          g_free (opt);
        }

        uploader->_bytes_uploaded = 0;

        {
          CURLcode result = curl_easy_perform (curl);

          if (result != CURLE_OK)
          {
            g_print ("%s\n", curl_easy_strerror (result));
          }
        }

        curl_easy_cleanup (curl);
      }
    }

    delete (uploader);
    return NULL;
  }

  // --------------------------------------------------------------------------------
  int Uploader::OnUpLoadTrace (CURL          *handle,
                               curl_infotype  type,
                               char          *data,
                               size_t         size,
                               Uploader      *uploader)
  {
    if (type == CURLINFO_TEXT)
    {
      g_print ("Uploader: ");
    }
    else if (type == CURLINFO_HEADER_IN)
    {
      g_print ("--CURLINFO_HEADER_IN------\n");
    }
    else if (type == CURLINFO_HEADER_OUT)
    {
      g_print ("--CURLINFO_HEADER_OUT-----\n");
    }
    else if (type == CURLINFO_DATA_IN)
    {
      g_print ("--CURLINFO_DATA_IN--------\n");
    }
    else if (type == CURLINFO_DATA_OUT)
    {
      g_print ("--CURLINFO_DATA_OUT-------\n");
    }

    g_print ("%s\n", data);

    return 0;
  }

  // --------------------------------------------------------------------------------
  size_t Uploader::ReadCallback (void     *ptr,
                                 size_t    size,
                                 size_t    nmemb,
                                 Uploader *uploader)
  {
    guint remaining_bytes = uploader->_data_length - uploader->_bytes_uploaded;
    guint bytes_to_copy   = MIN ((size*nmemb), remaining_bytes);

    memcpy (ptr,
            uploader->_data + uploader->_bytes_uploaded,
            bytes_to_copy);

    uploader->_bytes_uploaded += bytes_to_copy;

    return bytes_to_copy;
  }
}