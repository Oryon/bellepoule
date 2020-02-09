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
  FileUploader::FileUploader (const gchar *url,
                              const gchar *user,
                              const gchar *passwd,
                              const gchar *www)
  {
    _full_url  = nullptr;
    _user      = g_strdup (user);
    _passwd    = g_strdup (passwd);
    _file_path = nullptr;
    _url       = g_strdup (url);
    _www       = g_strdup (url);
  }

  // --------------------------------------------------------------------------------
  FileUploader::~FileUploader ()
  {
    printf (RED "FileUploader ***************>> Release\n" ESC);
    g_free (_user);
    g_free (_passwd);
    g_free (_file_path);
    g_free (_full_url);
    g_free (_url);
    g_free (_www);
  }

  // --------------------------------------------------------------------------------
  void FileUploader::UploadFile (const gchar *file_path)
  {
    _file_path = g_strdup (file_path);
    if (_file_path)
    {
      gchar *base_name = g_path_get_basename (file_path);
      gchar *escaped   = g_uri_escape_string (base_name,
                                              nullptr,
                                              FALSE);

      _full_url = g_strdup_printf ("%s/%s", _url, escaped);
      g_free (escaped);
      g_free (base_name);
    }

    Retain ();

    {
      GError *error = nullptr;

      GThread *sender_thread = g_thread_try_new ("FileUploader",
                                                 (GThreadFunc) ThreadFunction,
                                                 this,
                                                 &error);

      if (sender_thread == nullptr)
      {
        g_printerr ("Failed to create FileUploader thread: %s\n", error->message);
        g_error_free (error);

        Release ();
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
    return _full_url;
  }

  // --------------------------------------------------------------------------------
  const gchar *FileUploader::GetWWW ()
  {
    return _www;
  }

  // --------------------------------------------------------------------------------
  void FileUploader::Looper ()
  {
    if (_file_path)
    {
      GError *error = nullptr;
      gchar  *data_copy;

      if (g_file_get_contents (_file_path,
                               &data_copy,
                               nullptr,
                               &error) == FALSE)
      {
        g_print ("Unable to read file: %s\n", error->message);
        g_error_free (error);
      }
      else
      {
        SetDataCopy (data_copy);
        Upload ();
      }
    }
  }

  // --------------------------------------------------------------------------------
  gpointer FileUploader::ThreadFunction (FileUploader *uploader)
  {
    uploader->Looper  ();

    g_idle_add ((GSourceFunc) OnThreadDone,
                uploader);

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  gboolean FileUploader::OnThreadDone (FileUploader *uploader)
  {
    uploader->Release ();
    return G_SOURCE_REMOVE;
  }
}
