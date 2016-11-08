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
  Uploader::Uploader ()
    : Object ("Uploader")
  {
    _data        = NULL;
    _data_length = 0;
    _curl        = NULL;
  }

  // --------------------------------------------------------------------------------
  Uploader::~Uploader ()
  {
    g_free (_data);

    if (_curl)
    {
      curl_easy_cleanup (_curl);
    }
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
      g_print (BLUE "[Uploader] " ESC);
    }
    else if (type == CURLINFO_HEADER_IN)
    {
      g_print (GREEN "--CURLINFO_HEADER_IN------\n" ESC);
    }
    else if (type == CURLINFO_HEADER_OUT)
    {
      g_print (GREEN "--CURLINFO_HEADER_OUT-----\n" ESC);
    }
    else if (type == CURLINFO_DATA_IN)
    {
      g_print (GREEN "--CURLINFO_DATA_IN--------\n" ESC);
    }
    else if (type == CURLINFO_DATA_OUT)
    {
      g_print (GREEN "--CURLINFO_DATA_OUT-------\n" ESC);
      g_print ("......\n");
      return 0;
    }

    if (data && size)
    {
      gchar *printable_buffer = g_strndup (data, size);

      g_print ("%s", printable_buffer);
      if (printable_buffer[size-1] != '\n')
      {
        g_print ("\n");
      }
      g_free (printable_buffer);
    }

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

  // --------------------------------------------------------------------------------
  void Uploader::SetCurlOptions (CURL *curl)
  {
    curl_easy_setopt (_curl, CURLOPT_URL, GetUrl ());

    if (_data)
    {
      curl_easy_setopt (curl, CURLOPT_READFUNCTION,     ReadCallback);
      curl_easy_setopt (curl, CURLOPT_READDATA,         this);
      curl_easy_setopt (curl, CURLOPT_UPLOAD,           1L);
      curl_easy_setopt (curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t) _data_length);
    }

#ifdef UPLOADER_DEBUG
    curl_easy_setopt (curl, CURLOPT_DEBUGFUNCTION, OnUpLoadTrace);
    curl_easy_setopt (curl, CURLOPT_DEBUGDATA,     this);
    curl_easy_setopt (curl, CURLOPT_VERBOSE,       1L);
#endif
  }

  // --------------------------------------------------------------------------------
  const gchar *Uploader::GetUrl ()
  {
    return NULL;
  }

  // --------------------------------------------------------------------------------
  void Uploader::SetDataCopy (gchar *data)
  {
    _data        = data;
    _data_length = strlen (_data);
  }

  // --------------------------------------------------------------------------------
  CURLcode Uploader::Upload ()
  {
    CURLcode curl_code = CURLE_FAILED_INIT;

    _curl = curl_easy_init ();
    SetCurlOptions (_curl);

    if (_curl)
    {
      _bytes_uploaded = 0;

      curl_code = curl_easy_perform (_curl);

      g_free (_data);
      _data = NULL;
    }

    return curl_code;
  }
}
