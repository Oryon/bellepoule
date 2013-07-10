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

#ifdef WIN32
#define RED     ""
#define GREEN   ""
#define YELLOW  ""
#define BLUE    ""
#define MAGENTA ""
#define CYAN    ""
#define WHITE   ""
#define END     ""
#else
#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE    "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN    "\033[1;36m"
#define WHITE   "\033[0;37m"
#define END     "\033[0m"
#endif

namespace Net
{
  // --------------------------------------------------------------------------------
  Uploader::Uploader (const gchar  *url,
                      UploadStatus  status_cbk,
                      Object       *status_object,
                      const gchar  *user,
                      const gchar  *passwd)
    : Object ("Uploader")
  {
    _url    = g_strdup (url);
    _user   = g_strdup (user);
    _passwd = g_strdup (passwd);

    _status_cbk        = status_cbk;
    _status_cbk_object = status_object;

    if (_status_cbk_object)
    {
      _status_cbk_object->Retain ();
    }

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

    Object::TryToRelease (_status_cbk_object);
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

    Retain ();

    if (!g_thread_create ((GThreadFunc) ThreadFunction,
                          this,
                          FALSE,
                          &error))
    {
      g_printerr ("Failed to create Uploader thread: %s\n", error->message);
      g_error_free (error);
      Release ();
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
      g_print (BLUE "[Uploader] " END);
    }
    else if (type == CURLINFO_HEADER_IN)
    {
      g_print (GREEN "--CURLINFO_HEADER_IN------\n" END);
    }
    else if (type == CURLINFO_HEADER_OUT)
    {
      g_print (GREEN "--CURLINFO_HEADER_OUT-----\n" END);
    }
    else if (type == CURLINFO_DATA_IN)
    {
      g_print (GREEN "--CURLINFO_DATA_IN--------\n" END);
    }
    else if (type == CURLINFO_DATA_OUT)
    {
      g_print (GREEN "--CURLINFO_DATA_OUT-------\n" END);
      g_print ("......\n");
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
  gboolean Uploader::DeferedStatus (Uploader *uploader)
  {
    uploader->_status_cbk (uploader->_peer_status,
                           uploader->_status_cbk_object);
    uploader->Release ();

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  gpointer Uploader::ThreadFunction (Uploader *uploader)
  {
    gboolean defered_operation = FALSE;

    if (uploader->_data && uploader->_full_url)
    {
      CURL *curl = curl_easy_init ();

      if (curl)
      {
        curl_easy_setopt (curl, CURLOPT_READFUNCTION,     ReadCallback);
        curl_easy_setopt (curl, CURLOPT_READDATA,         uploader);
        curl_easy_setopt (curl, CURLOPT_UPLOAD,           1L);
        curl_easy_setopt (curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t) strlen (uploader->_data) + 1);
        curl_easy_setopt (curl, CURLOPT_URL,              uploader->_full_url);
        curl_easy_setopt (curl, CURLOPT_NOPROXY,          "127.0.0.1");
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
          CURLcode curl_code;

          curl_code = curl_easy_perform (curl);
          if (curl_code != CURLE_OK)
          {
            uploader->_peer_status = CONN_ERROR;
            g_print (RED "[Uploader Error] " END "%s\n", curl_easy_strerror (curl_code));
          }
          else
          {
            uploader->_peer_status = CONN_OK;
            g_print (YELLOW "[Uploader] " END "Done");
          }

          if (uploader->_status_cbk && uploader->_status_cbk_object)
          {
            defered_operation = TRUE;
            g_idle_add ((GSourceFunc) DeferedStatus,
                        uploader);
          }
        }

        curl_easy_cleanup (curl);
      }
    }

    if (defered_operation == FALSE)
    {
      uploader->Release ();
    }

    return NULL;
  }
}
