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

#include "uploader.hpp"

namespace Net
{
  class Message;
  class Cryptor;

  class MessageUploader : public Uploader
  {
    public:
      enum class PeerStatus
      {
        CONN_OK,
        CONN_ERROR,
        CONN_CLOSED
      };

      struct Listener
      {
        virtual void OnUploadStatus (PeerStatus peer_status) = 0;
      };

    public:
      MessageUploader (const gchar *url,
                       Listener    *listener = nullptr);

      void Stop ();

      void PushMessage (Message *message);

    protected:
      ~MessageUploader () override;

    private:
      struct ThreadData
      {
        MessageUploader *_uploader;
        Message         *_message;
        PeerStatus       _peer_status;
      };

      gchar             *_url;
      gchar             *_iv;
      Listener          *_listener;
      GThread           *_sender_thread;
      GAsyncQueue       *_message_queue;
      Cryptor           *_cryptor;
      struct curl_slist *_http_header;

      static gpointer ThreadFunction (MessageUploader *uploader);

      static gboolean OnMessageUsed (ThreadData *data);

      void PrepareData (gchar       *data_copy,
                        const gchar *passphrase);

    private:
      void SetCurlOptions (CURL *curl) override;

      const gchar *GetUrl () override;
  };

}
