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
  Body::Body ()
    : Object ("Upload::Body")
  {
    _content        = g_new (gchar, 1);
    _size           = 0;
    _size_processed = 0;
  }

  // --------------------------------------------------------------------------------
  Body::~Body ()
  {
    Clear ();
  }

  // --------------------------------------------------------------------------------
  const gchar *Body::GetContent ()
  {
    return _content;
  }

  // --------------------------------------------------------------------------------
  void Body::Clear ()
  {
    g_free (_content);
    _content        = NULL;
    _size           = 0;
    _size_processed = 0;
  }

  // --------------------------------------------------------------------------------
  void Body::SetContent (const gchar *content)
  {
    Clear ();

    _content = g_strdup (content);
    _size    = strlen (content);
  }

  // --------------------------------------------------------------------------------
  size_t Body::GetSize ()
  {
    return _size;
  }

  // --------------------------------------------------------------------------------
  size_t Body::CopyTo (void   *to,
                       size_t  size,
                       size_t  nmemb)
  {
    guint remaining_bytes = _size - _size_processed;
    guint bytes_to_copy   = MIN ((size*nmemb), remaining_bytes);

    memcpy (to,
            _content + _size_processed,
            bytes_to_copy);

    _size_processed += bytes_to_copy;

    return bytes_to_copy;
  }

  // --------------------------------------------------------------------------------
  size_t Body::CopyFrom (void   *from,
                         size_t  size,
                         size_t  nmemb)
  {
    size_t realsize = size * nmemb;

    _content = (gchar *) g_realloc (_content,
                                    _size + realsize + 1);

    memcpy (&(_content[_size]),
            from,
            realsize);

    _size += realsize;
    _content[_size] = 0;

    return realsize;
  }
}

namespace Net
{
  // --------------------------------------------------------------------------------
  Uploader::Uploader ()
    : Object ("Uploader")
  {
    _body_out = new Body ();
    _body_in  = new Body ();
    _curl        = NULL;
  }

  // --------------------------------------------------------------------------------
  Uploader::~Uploader ()
  {
    _body_in->Release ();
    _body_out->Release ();

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
    return uploader->_body_out->CopyTo (ptr,
                                        size,
                                        nmemb);
  }

  // --------------------------------------------------------------------------------
  size_t Uploader::WriteCallback (void     *ptr,
                                  size_t    size,
                                  size_t    nmemb,
                                  Uploader *uploader)
  {
    return uploader->_body_in->CopyFrom (ptr,
                                         size,
                                         nmemb);
  }

  // --------------------------------------------------------------------------------
  void Uploader::SetCurlOptions (CURL *curl)
  {
    curl_easy_setopt (_curl, CURLOPT_URL, GetUrl ());

    if (_body_out->GetSize () > 0)
    {
      curl_easy_setopt (curl, CURLOPT_READFUNCTION,     ReadCallback);
      curl_easy_setopt (curl, CURLOPT_READDATA,         this);
      curl_easy_setopt (curl, CURLOPT_UPLOAD,           1L);
      curl_easy_setopt (curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t) _body_out->GetSize ());
    }

    curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt (curl, CURLOPT_WRITEDATA,     this);

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
  void Uploader::OnUploadDone (const gchar *response)
  {
  }

  // --------------------------------------------------------------------------------
  void Uploader::SetDataCopy (gchar *data)
  {
    _body_out->SetContent (data);
    g_free (data);
  }

  // --------------------------------------------------------------------------------
  CURLcode Uploader::Upload ()
  {
    CURLcode curl_code = CURLE_FAILED_INIT;

    _curl = curl_easy_init ();
    SetCurlOptions (_curl);

    if (_curl)
    {
      _body_in->Clear ();

      curl_code = curl_easy_perform (_curl);

      OnUploadDone (_body_in->GetContent ());
    }

    return curl_code;
  }
}
