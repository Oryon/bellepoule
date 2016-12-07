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

#include "cryptor.hpp"
#include "message.hpp"
#include "message_uploader.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  MessageUploader::MessageUploader (const gchar *url,
                                    Listener    *listener)
  {
    _url = g_strdup (url);

    _listener = listener;
    if (_listener)
    {
      _listener->Use ();
    }

    _iv      = NULL;
    _cryptor = new Net::Cryptor ();

    _http_header = NULL;

    _message_queue = g_async_queue_new_full ((GDestroyNotify) Object::TryToRelease);

    {
      GError *error = NULL;

      _sender_thread = g_thread_try_new ("MessageUploader",
                                         (GThreadFunc) Loop,
                                         this,
                                         &error);
      if (_sender_thread == NULL)
      {
        g_printerr ("Failed to create Uploader thread: %s\n", error->message);
        g_error_free (error);
      }
    }
  }

  // --------------------------------------------------------------------------------
  MessageUploader::~MessageUploader ()
  {
    g_async_queue_unref (_message_queue);

    if (_sender_thread)
    {
      Message *stop_sending = new Message ("MessageUploader::stop_sending");

      PushMessage (stop_sending);
      stop_sending->Release ();

      g_thread_join (_sender_thread);
    }

    g_free (_url);

    g_free (_iv);
    _cryptor->Release ();

    if (_listener)
    {
      _listener->Drop ();
    }

    if (_http_header)
    {
      curl_slist_free_all (_http_header);
    }
  }

  // --------------------------------------------------------------------------------
  void MessageUploader::PrepareData (gchar       *data_copy,
                                     const gchar *passphrase)
  {
    if (passphrase)
    {
      guchar *iv;
      char   *crypted;

      crypted = _cryptor->Encrypt (data_copy,
                                   passphrase,
                                   &iv);
      SetDataCopy (crypted);
      g_free (data_copy);

      _iv = g_base64_encode (iv, 16);
      g_free (iv);
    }
    else
    {
      SetDataCopy (data_copy);
      _iv = NULL;
    }
  }

  // --------------------------------------------------------------------------------
  void MessageUploader::PushMessage (Message *message)
  {
    if (message->IsWaitingToBeSent () == FALSE)
    {
      message->MarkAsWaitingToBeSent ();
      message->Retain ();
      g_async_queue_push (_message_queue,
                          message);
    }
  }

  // --------------------------------------------------------------------------------
  void MessageUploader::SetCurlOptions (CURL *curl)
  {
    Uploader::SetCurlOptions (curl);

    {
      if (_http_header)
      {
        curl_slist_free_all (_http_header);
        _http_header = NULL;
      }

      if (_iv)
      {
        gchar *iv_header = g_strdup_printf ("IV: %s", _iv);

        _http_header = curl_slist_append (_http_header, iv_header);
        g_free (iv_header);
      }

      curl_easy_setopt (curl, CURLOPT_HTTPHEADER, _http_header);
    }
  }

  // --------------------------------------------------------------------------------
  const gchar *MessageUploader::GetUrl ()
  {
    return _url;
  }

  // --------------------------------------------------------------------------------
  gboolean MessageUploader::DeferedStatus (MessageUploader *uploader)
  {
    uploader->_listener->OnUploadStatus (uploader->_peer_status);
    uploader->Release ();

    return G_SOURCE_REMOVE;
  }

  // --------------------------------------------------------------------------------
  gpointer MessageUploader::Loop (MessageUploader *uploader)
  {
    g_async_queue_ref (uploader->_message_queue);

    while (1)
    {
      {
        Message *message = (Message *) g_async_queue_pop (uploader->_message_queue);

        message->MarkAsSent ();

        if (message->Is ("MessageUploader::stop_sending"))
        {
          message->Release ();
          break;
        }
        else if (message->Is ("MessageUploader::raw_content"))
        {
          uploader->PrepareData (message->GetString ("content"),
                                 message->GetPassPhrase ());
        }
        else
        {
          uploader->PrepareData (message->GetParcel (),
                                 message->GetPassPhrase ());
        }

        message->Release ();
      }


      {
        CURLcode curl_code = uploader->Upload ();

        if (curl_code != CURLE_OK)
        {
          uploader->_peer_status = CONN_ERROR;
          g_print (RED "[Uploader Error] " ESC "%s\n", curl_easy_strerror (curl_code));
        }
        else
        {
          uploader->_peer_status = CONN_OK;
#ifdef UPLOADER_DEBUG
          g_print (YELLOW "[Uploader] " ESC "Done\n");
#endif
        }

        if (uploader->_listener)
        {
          g_idle_add ((GSourceFunc) DeferedStatus,
                      uploader);
        }
      }

      g_free (uploader->_iv);
      uploader->_iv = NULL;
    }

    g_async_queue_unref (uploader->_message_queue);

    return NULL;
  }
}
