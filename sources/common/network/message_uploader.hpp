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

#ifndef message_upload_hpp
#define message_upload_hpp

#include "uploader.hpp"

namespace Net
{
  class Message;
  class Cryptor;

  class MessageUploader : public Uploader
  {
    public:
      typedef enum
      {
        CONN_OK,
        CONN_ERROR
      } PeerStatus;

      class Listener
      {
        public:
          virtual void OnUploadStatus (PeerStatus peer_status) = 0;
          virtual void Use  () = 0;
          virtual void Drop () = 0;
      };

    public:
      MessageUploader (const gchar *url,
                       Listener    *listener = NULL);

      virtual void PushMessage (Message *message);

    protected:
      virtual ~MessageUploader ();

    private:
      gchar       *_url;
      gchar       *_iv;
      Listener    *_listener;
      PeerStatus   _peer_status;
      GThread     *_sender_thread;
      GAsyncQueue *_message_queue;
      Cryptor     *_cryptor;

      static gpointer Loop (MessageUploader *uploader);

      void PrepareData (gchar       *data_copy,
                        const gchar *passphrase);

      static gboolean DeferedStatus (MessageUploader *uploader);

    private:
      struct curl_slist *SetHeader (struct curl_slist *list);

      const gchar *GetUrl ();
  };

}
#endif

