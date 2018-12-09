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

    _iv      = nullptr;
    _cryptor = new Net::Cryptor ();

    _http_header = nullptr;

    _message_queue = g_async_queue_new_full ((GDestroyNotify) Object::TryToRelease);

    {
      GError *error = nullptr;

      _sender_thread = g_thread_try_new ("MessageUploader",
                                         (GThreadFunc) ThreadFunction,
                                         this,
                                         &error);
      if (_sender_thread == nullptr)
      {
        g_printerr ("Failed to create Uploader thread: %s\n", error->message);
        g_error_free (error);

        Release ();
      }
    }
  }

  // --------------------------------------------------------------------------------
  MessageUploader::~MessageUploader ()
  {
    g_async_queue_unref (_message_queue);

    g_free (_url);

    g_free (_iv);
    _cryptor->Release ();

    if (_http_header)
    {
      curl_slist_free_all (_http_header);
    }
  }

  // --------------------------------------------------------------------------------
  void MessageUploader::Stop ()
  {
    if (_sender_thread)
    {
      Message *stop_sending = new Message ("MessageUploader::stop_sending");

      PushMessage (stop_sending);
      stop_sending->Release ();
    }
  }

  // --------------------------------------------------------------------------------
  void MessageUploader::PrepareData (gchar       *data_copy,
                                     const gchar *passphrase)
  {
    if (data_copy == nullptr)
    {
      g_error ("MessageUploader::PrepareData ==> data_copy NULL");
    }

    if (passphrase)
    {
      char *crypted = _cryptor->Encrypt (data_copy,
                                         passphrase,
                                         &_iv);
      SetDataCopy (crypted);
      g_free (data_copy);
    }
    else
    {
      SetDataCopy (data_copy);
      _iv = nullptr;
    }
  }

  // --------------------------------------------------------------------------------
  void MessageUploader::PushMessage (Message *message)
  {
    Message *clone = message->Clone ();

    g_async_queue_push (_message_queue,
                        clone);
  }

  // --------------------------------------------------------------------------------
  void MessageUploader::SetCurlOptions (CURL *curl)
  {
    Uploader::SetCurlOptions (curl);

    {
      if (_http_header)
      {
        curl_slist_free_all (_http_header);
        _http_header = nullptr;
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
  gboolean MessageUploader::OnMessageUsed (ThreadData *data)
  {
    PeerStatus       peer_status = data->_peer_status;
    MessageUploader *uploader    = data->_uploader;

    data->_message->Release ();
    g_free (data);

    if (peer_status != PeerStatus::CONN_OK)
    {
      uploader->_listener->OnUploadStatus (peer_status);
    }

    return G_SOURCE_REMOVE;
  }

  // --------------------------------------------------------------------------------
  gpointer MessageUploader::ThreadFunction (MessageUploader *uploader)
  {
    while (1)
    {
      ThreadData *data = new ThreadData ();

      data->_uploader    = uploader;
      data->_peer_status = PeerStatus::CONN_OK;
      data->_message     = (Message *) g_async_queue_pop (uploader->_message_queue);

      if (data->_message->Is ("MessageUploader::stop_sending"))
      {
        data->_peer_status = PeerStatus::CONN_CLOSED;

        g_idle_add ((GSourceFunc) OnMessageUsed,
                    data);
        break;
      }
      else
      {
        uploader->PrepareData (data->_message->GetParcel (),
                               data->_message->GetPassPhrase256 ());
      }

      {
        CURLcode curl_code = uploader->Upload ();

        if (curl_code != CURLE_OK)
        {
          data->_peer_status = PeerStatus::CONN_ERROR;
          g_print (RED "[Uploader Error] " ESC "%s\n", curl_easy_strerror (curl_code));
        }
        else
        {
          data->_peer_status = PeerStatus::CONN_OK;
#ifdef UPLOADER_DEBUG
          g_print (YELLOW "[Uploader] " ESC "Done\n");
#endif
        }

        g_idle_add ((GSourceFunc) OnMessageUsed,
                    data);
      }

      g_free (uploader->_iv);
      uploader->_iv = nullptr;
    }

    return nullptr;
  }
}
