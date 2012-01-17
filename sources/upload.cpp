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

#include "upload.hpp"

// --------------------------------------------------------------------------------
Upload::Upload (const gchar *filename,
                const gchar *url,
                const gchar *user,
                const gchar *passwd)
{
  _user     = g_strdup (user);
  _passwd   = g_strdup (passwd);

  _full_url = NULL;
  if (filename && url && *url)
  {
    gchar *base_name  = g_path_get_basename (filename);

    _full_url  = g_strdup_printf ("%s/%s", url, base_name);
    g_free (base_name);
  }

  _data = NULL;
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
}

// --------------------------------------------------------------------------------
Upload::~Upload ()
{
  g_free (_full_url);
  g_free (_user);
  g_free (_passwd);
  g_free (_data);
}

// --------------------------------------------------------------------------------
void Upload::Start ()
{
  GError *error = NULL;

  if (!g_thread_create ((GThreadFunc) ThreadFunction,
                        this,
                        FALSE,
                        &error))
  {
    g_printerr ("Failed to create Upload thread: %s\n", error->message);
    g_free (error);
    delete (this);
  }
}

// --------------------------------------------------------------------------------
gpointer Upload::ThreadFunction (Upload *upload)
{
  if (upload->_data && upload->_full_url)
  {
    CURL *curl = curl_easy_init ();

    if (curl)
    {
      curl_easy_setopt (curl, CURLOPT_READFUNCTION,  ReadCallback);
      curl_easy_setopt (curl, CURLOPT_READDATA,      upload);
      curl_easy_setopt (curl, CURLOPT_UPLOAD,        1L);
      curl_easy_setopt (curl, CURLOPT_URL,           upload->_full_url);
      curl_easy_setopt (curl, CURLOPT_DEBUGFUNCTION, OnUpLoadTrace);
      curl_easy_setopt (curl, CURLOPT_DEBUGDATA,     upload);
      curl_easy_setopt (curl, CURLOPT_VERBOSE,       1L);

      if (upload->_user && *upload->_user && upload->_passwd && *upload->_passwd)
      {
        gchar *opt = g_strdup_printf ("%s:%s", upload->_user, upload->_passwd);

        curl_easy_setopt (curl, CURLOPT_USERPWD, opt);
        g_free (opt);
      }

      upload->_bytes_uploaded = 0;

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

  delete (upload);
  return NULL;
}

// --------------------------------------------------------------------------------
int Upload::OnUpLoadTrace (CURL          *handle,
                           curl_infotype  type,
                           char          *data,
                           size_t         size,
                           Upload        *upload)
{
  if (type == CURLINFO_TEXT)
  {
    g_print ("FTP Upload: %s", data);
  }

  return 0;
}

// --------------------------------------------------------------------------------
size_t Upload::ReadCallback (void   *ptr,
                             size_t  size,
                             size_t  nmemb,
                             Upload *upload)
{
  guint remaining_bytes = upload->_data_length - upload->_bytes_uploaded;
  guint bytes_to_copy   = MIN ((size*nmemb), remaining_bytes);

  memcpy (ptr,
          upload->_data + upload->_bytes_uploaded,
          bytes_to_copy);

  upload->_bytes_uploaded += bytes_to_copy;

  return bytes_to_copy;
}
