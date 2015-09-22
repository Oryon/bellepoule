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

#ifndef file_upload_hpp
#define file_upload_hpp

#include "uploader.hpp"

namespace Net
{
  class FileUploader : public Uploader
  {
    public:
      FileUploader (const gchar  *url,
                    const gchar  *user,
                    const gchar  *passwd);

      void UploadFile (const gchar *filename);

    private:
      gchar *_user;
      gchar *_passwd;
      gchar *_drop_dir;
      gchar *_url;
      gchar *_filename;

      virtual ~FileUploader ();

      void PushMessage (Message *message);

      void SetCurlOptions (CURL *curl);

      const gchar *GetUrl ();

      static gpointer SenderThread (FileUploader *uploader);
  };

}
#endif

