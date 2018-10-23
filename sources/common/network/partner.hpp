// Copyright (C) 2009 Yannick Le Roux.
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

#include "util/object.hpp"
#include "message_uploader.hpp"

namespace Net
{
  class Message;

  class Partner : public Object,
                  public Net::MessageUploader::Listener
  {
    public:
      struct Listener
      {
        virtual void OnPartnerKilled (Partner *partener) = 0;
        virtual void OnPartnerLeaved (Partner *partener) = 0;
      };

    public:
      Partner (Message  *message,
               Listener *listener);

      void SetPassPhrase256 (const gchar *passphrase256);

      void SendMessage (Message *message);

      gboolean Is (gint32 partner_id);

      gboolean HasRole (guint role);

      const gchar *GetAddress ();

      guint GetPort ();

      void Leave ();

    private:
      guint            _role;
      gint32           _partner_id;
      gchar           *_address;
      guint            _port;
      MessageUploader *_uploader;
      gchar           *_passphrase256;
      Listener        *_listener;

      ~Partner ();

      void OnUploadStatus (MessageUploader::PeerStatus peer_status);
  };
}
