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

#include <stdlib.h>

#include "message_uploader.hpp"
#include "message.hpp"
#include "partner.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  Partner::Partner (Message *message)
    : Object ("Partner")
  {
    _role = message->GetString ("role");

    _passphrase256 = NULL;

    {
      gchar *ip   = message->GetString ("ip_address");
      guint  port = message->GetInteger ("unicast_port");

      if (ip)
      {
        gchar *url = g_strdup_printf ("http://%s:%d/ring", ip, port);

        _uploader = new Net::MessageUploader (url);
        _address  = g_strdup (ip);
        _port     = port;

        g_free (url);
        g_free (ip);
      }
    }
  }

  // --------------------------------------------------------------------------------
  Partner::~Partner ()
  {
    g_free (_role);
    g_free (_address);
    g_free (_passphrase256);

    _uploader->Stop ();
  }

  // --------------------------------------------------------------------------------
  gboolean Partner::HasRole (const gchar *role)
  {
    return (g_strcmp0 (_role, _role) == 0);
  }

  // --------------------------------------------------------------------------------
  const gchar *Partner::GetRole ()
  {
    return _role;
  }

  // --------------------------------------------------------------------------------
  gboolean Partner::Is (Partner *partner)
  {
    return HasRole (partner->_role);
  }

  // --------------------------------------------------------------------------------
  void Partner::SetPassPhrase256 (const gchar *passphrase256)
  {
    g_free (_passphrase256);
    _passphrase256 = g_strdup (passphrase256);
  }

  // --------------------------------------------------------------------------------
  void Partner::SendMessage (Message *message)
  {
    message->Dump (FALSE);

    message->SetPassPhrase256 (_passphrase256);
    _uploader->PushMessage (message);
  }

  // --------------------------------------------------------------------------------
  const gchar *Partner::GetAddress ()
  {
    return _address;
  }

  // --------------------------------------------------------------------------------
  guint Partner::GetPort ()
  {
    return _port;
  }
}
