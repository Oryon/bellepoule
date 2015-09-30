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

#include "file_uploader.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  FileUploader::FileUploader (const gchar  *url,
                              const gchar  *user,
                              const gchar  *passwd)
  {
    _drop_dir = g_strdup (url);;
    _user     = g_strdup (user);
    _passwd   = g_strdup (passwd);
    _filename = NULL;
    _url      = NULL;
  }

  // --------------------------------------------------------------------------------
  FileUploader::~FileUploader ()
  {
    printf (RED "FileUploader ***************>> Release\n" ESC);
    g_free (_user);
    g_free (_passwd);
    g_free (_filename);
    g_free (_drop_dir);
    g_free (_url);
  }

  // --------------------------------------------------------------------------------
  void FileUploader::UploadFile (const gchar *filename)
  {
    _filename = g_strdup (filename);

    if (_filename && _drop_dir)
    {
      gchar *base_name = g_path_get_basename (_filename);

      _url = g_strdup_printf ("%s/%s", _drop_dir, base_name);
      g_free (base_name);
    }


    {
      GError *error = NULL;

      GThread *sender_thread = g_thread_create ((GThreadFunc) SenderThread,
                                                this,
                                                FALSE,
                                                &error);

      if (sender_thread == NULL)
      {
        g_printerr ("Failed to create Uploader thread: %s\n", error->message);
        g_error_free (error);
      }
      else
      {
        Retain ();
      }
    }
  }

  // --------------------------------------------------------------------------------
  void FileUploader::SetCurlOptions (CURL *curl)
  {
    Uploader::SetCurlOptions (curl);

    if (_user && *_user && _passwd && *_passwd)
    {
      gchar *opt = g_strdup_printf ("%s:%s", _user, _passwd);

      curl_easy_setopt (curl, CURLOPT_USERPWD, opt);
      g_free (opt);
    }
  }

  // --------------------------------------------------------------------------------
  const gchar *FileUploader::GetUrl ()
  {
    return _url;
  }

  // --------------------------------------------------------------------------------
  gpointer FileUploader::SenderThread (FileUploader *uploader)
  {
    uploader->Init ();

    if (uploader->_filename)
    {
      GError *error = NULL;
      gchar  *data_copy;

      if (g_file_get_contents (uploader->_filename,
                               &data_copy,
                               NULL,
                               &error) == FALSE)
      {
        g_print ("Unable to read file: %s\n", error->message);
        g_error_free (error);
      }
      else
      {
        uploader->SetDataCopy (data_copy);
        uploader->Upload ();
      }
    }

    uploader->Release ();
    return NULL;
  }
}
