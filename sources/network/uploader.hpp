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

#ifndef upload_hpp
#define upload_hpp

#include <curl/curl.h>
#include <glib.h>

namespace Net
{
  class Uploader
  {
    public:
      Uploader (const gchar *url,
                const gchar *user,
                const gchar *passwd);

      void UploadFile (const gchar *filename);

      void UploadString (const gchar *string);

    private:
      gchar *_user;
      gchar *_passwd;
      gchar *_full_url;
      gchar *_url;
      gchar *_data;
      gsize  _data_length;
      guint  _bytes_uploaded;

      virtual ~Uploader ();

      void Start ();

      static gpointer ThreadFunction (Uploader *uploader);

      static int OnUpLoadTrace (CURL          *handle,
                                curl_infotype  type,
                                char          *data,
                                size_t         size,
                                Uploader      *uploader);

      static size_t ReadCallback (void     *ptr,
                                  size_t    size,
                                  size_t    nmemb,
                                  Uploader *uploader);
  };

}
#endif
