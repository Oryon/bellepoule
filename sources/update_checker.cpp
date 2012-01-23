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

#include "update_checker.hpp"

GSourceFunc  UpdateChecker::_callback;
gpointer     UpdateChecker::_callback_data;
gchar       *UpdateChecker::_latest = NULL;

// --------------------------------------------------------------------------------
size_t UpdateChecker::AddText (void   *contents,
                               size_t  size,
                               size_t  nmemb,
                               Data   *data)
{
  size_t realsize = size * nmemb;

  data->text = (char *) g_realloc (data->text,
                                   data->size + realsize + 1);
  if (data->text)
  {
    memcpy (&(data->text[data->size]),
            contents,
            realsize);

    data->size += realsize;
    data->text[data->size] = 0;
  }

  return realsize;
}

// --------------------------------------------------------------------------------
gpointer UpdateChecker::ThreadFunction (gpointer thread_data)
{
  CURL *curl_handle;
  Data  data;

  data.text = (char *) g_malloc (1);
  data.size = 0;

  curl_handle = curl_easy_init ();

  curl_easy_setopt (curl_handle,
                    CURLOPT_URL,
                    "http://betton.escrime.free.fr/documents/BellePoule/latest.html");

  curl_easy_setopt (curl_handle,
                    CURLOPT_WRITEFUNCTION,
                    AddText);

  curl_easy_setopt (curl_handle,
                    CURLOPT_WRITEDATA,
                    (void *) &data);

  curl_easy_setopt (curl_handle,
                    CURLOPT_USERAGENT,
                    "libcurl-agent/1.0");

  curl_easy_perform (curl_handle);

  curl_easy_cleanup (curl_handle);

  _latest = data.text;

  g_idle_add (_callback,
              _callback_data);

  return NULL;
}

// --------------------------------------------------------------------------------
void UpdateChecker::DownloadLatestVersion (GSourceFunc callback,
                                           gpointer    callback_data)
{
  GError *error = NULL;

  _callback      = callback;
  _callback_data = callback_data;

  if (!g_thread_create ((GThreadFunc) ThreadFunction,
                        NULL,
                        FALSE,
                        &error))
  {
    g_printerr ("Failed to create UpdateChecker thread: %s\n", error->message);
    g_free (error);
  }
}

// --------------------------------------------------------------------------------
gboolean UpdateChecker::GetLatestVersion (GKeyFile *key_file)
{
  return g_key_file_load_from_data (key_file,
                                    _latest,
                                    strlen (_latest) + 1,
                                    G_KEY_FILE_NONE,
                                    NULL);
}
