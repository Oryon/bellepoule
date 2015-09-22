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
    _message_list = NULL;

    _role = message->GetString ("role");

    {
      gchar *ip   = message->GetString ("ip_address");
      guint  port = message->GetInteger ("unicast_port");

      if (ip)
      {
        gchar *url = g_strdup_printf ("http://%s:%d", ip, port);

        _uploader = new Net::MessageUploader (url);

        g_free (url);
        g_free (ip);
      }

      _address = g_strdup_printf ("%s:<b>%d</b>", ip, port);
    }
  }

  // --------------------------------------------------------------------------------
  Partner::~Partner ()
  {
    g_free (_role);
    g_free (_address);

    g_list_free_full (_message_list,
                      (GDestroyNotify) Object::TryToRelease);

    _uploader->Release ();
  }

  // --------------------------------------------------------------------------------
  gboolean Partner::HasRole (const gchar *role)
  {
    if (_role && role)
    {
      return (strcmp (_role, _role) == 0);
    }

    return (_role == _role);
  }

  // --------------------------------------------------------------------------------
  gboolean Partner::Is (Partner *partner)
  {
    return HasRole (partner->_role);
  }

  // --------------------------------------------------------------------------------
  void Partner::SendMessage (Message *message)
  {
    _uploader->PushMessage (message);
  }

  // --------------------------------------------------------------------------------
  void Partner::Store (Message *message)
  {
    _message_list = g_list_prepend (_message_list,
                                    message);
    message->Retain ();
  }

  // --------------------------------------------------------------------------------
  const gchar *Partner::GetAddress ()
  {
    return _address;
  }
}
