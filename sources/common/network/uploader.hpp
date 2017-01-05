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

#pragma once

#include <curl/curl.h>
#include <glib.h>
#include "util/object.hpp"

namespace Net
{
  class Body : public Object
  {
    public:
      Body ();

      void Clear ();

      void SetContent (const gchar *content);

      const gchar *GetContent ();

      size_t CopyTo (void   *to,
                     size_t  size,
                     size_t  nmemb);

      size_t CopyFrom (void   *from,
                       size_t  size,
                       size_t  nmemb);

      size_t GetSize ();

    private:
      gchar  *_content;
      size_t  _size;
      size_t  _size_processed;

      ~Body ();
  };

  class Uploader : public Object
  {
    public:
      Uploader ();

    protected:

      virtual ~Uploader ();

      CURLcode Upload ();

      void SetDataCopy (gchar *data);

    protected:
      virtual void SetCurlOptions (CURL *curl);

      virtual const gchar *GetUrl ();

      virtual void OnUploadDone (const gchar *response);

    private:
      CURL  *_curl;

      Body *_body_out;
      Body *_body_in;

      void PrepareData (gchar       *data_copy,
                        const gchar *passphrase);

      static int OnUpLoadTrace (CURL          *handle,
                                curl_infotype  type,
                                char          *data,
                                size_t         size,
                                Uploader      *uploader);

      static size_t ReadCallback (void     *ptr,
                                  size_t    size,
                                  size_t    nmemb,
                                  Uploader *uploader);

      static size_t WriteCallback (void     *ptr,
                                   size_t    size,
                                   size_t    nmemb,
                                   Uploader *uploader);
  };
}
