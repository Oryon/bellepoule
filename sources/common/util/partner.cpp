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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "partner.hpp"

// --------------------------------------------------------------------------------
Partner::Partner (const gchar        *who,
                  struct sockaddr_in *from)
  : Object ("Partner")
{
  gchar **slices = g_strsplit (who,
                               ":",
                               -1);

  _ip   = g_strdup (inet_ntoa (from->sin_addr));
  _role = g_strdup (slices[0]);
  _port = atoi (slices[1]);

  g_strfreev (slices);
}

// --------------------------------------------------------------------------------
Partner::~Partner ()
{
  g_free (_ip);
  g_free (_role);
}

// --------------------------------------------------------------------------------
void Partner::Accept ()
{
  SendMessage ("tagada soinsoin");
}

// --------------------------------------------------------------------------------
gboolean Partner::Is (const gchar *role)
{
  return (strcmp (role, _role) == 0);
}

// --------------------------------------------------------------------------------
void Partner::Use ()
{
  Retain ();
}

// --------------------------------------------------------------------------------
void Partner::Drop ()
{
  Release ();
}

// --------------------------------------------------------------------------------
void Partner::OnUploadStatus (Net::Uploader::PeerStatus peer_status)
{
}

// --------------------------------------------------------------------------------
gboolean Partner::SendMessage (const gchar *message)
{
  if (_ip)
  {
    Net::Uploader *uploader;

    {
      gchar *url = g_strdup_printf ("http://%s:%d", _ip, _port);

      uploader = new Net::Uploader (url,
                                    this,
                                    NULL, NULL);

      g_free (url);
    }

    {
      gchar *full_message = g_strdup_printf ("%s", message);

      uploader->UploadString (full_message,
                              NULL);

      g_free (full_message);
    }

    uploader->Release ();
    return TRUE;
  }

  return FALSE;
}
