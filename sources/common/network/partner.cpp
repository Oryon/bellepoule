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

#include "uploader.hpp"
#include "message.hpp"
#include "partner.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  Partner::Partner (Message *message)
    : Object ("Partner")
  {
    _message_list = NULL;

    _ip   = message->GetString   ("ip_address");
    _port = message->GetInteger  ("unicast_port");

    _role = message->GetString ("role");

    _address = g_strdup_printf ("%s:%d", _ip, _port);
  }

  // --------------------------------------------------------------------------------
  Partner::~Partner ()
  {
    g_free (_ip);
    g_free (_role);

    {
      GList *current = _message_list;

      while (current)
      {
        Net::Message *message = (Net::Message *) current->data;

        message->Dump ();

        current = g_list_next (current);
      }
    }

    g_list_free_full (_message_list,
                      (GDestroyNotify) Object::TryToRelease);
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
  gboolean Partner::SendMessage (Message *message)
  {
    if (_ip)
    {
      Net::Uploader *uploader;

      {
        gchar *url = g_strdup_printf ("http://%s:%d", _ip, _port);

        uploader = new Net::Uploader (url,
                                      NULL,
                                      NULL, NULL);

        g_free (url);
      }

      {
        gchar *parcel = message->GetParcel ();

        message->Dump ();
        uploader->UploadString (parcel,
                                NULL);
        g_free (parcel);
      }

      uploader->Release ();

      return TRUE;
    }

    return FALSE;
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
