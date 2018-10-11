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

#include <sys/types.h>
#include <qrencode.h>
#include <gio/gnetworking.h>

#include "network/ring.hpp"
#include "network/wifi_network.hpp"

#ifndef LIVE_POOL
#include "util/player.hpp"
#endif
#include "wifi_code.hpp"

Net::WifiNetwork *WifiCode::_wifi_network = NULL;
guint             WifiCode::_port         = 0;

// --------------------------------------------------------------------------------
WifiCode::WifiCode (const gchar *user_name)
  : FlashCode (user_name)
{
  _player = NULL;
  _key    = NULL;
}

// --------------------------------------------------------------------------------
WifiCode::WifiCode (Player *player)
  : FlashCode (NULL)
{
  _player = player;
  _key    = NULL;
}

// --------------------------------------------------------------------------------
WifiCode::~WifiCode ()
{
  g_free (_key);
}

// --------------------------------------------------------------------------------
void WifiCode::SetWifiNetwork (Net::WifiNetwork *network)
{
  _wifi_network = network;
}

// --------------------------------------------------------------------------------
guint WifiCode::ClaimIpPort ()
{
  GInetAddress *any_ip = g_inet_address_new_any (G_SOCKET_FAMILY_IPV4);
  GSocket      *socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
                                       G_SOCKET_TYPE_STREAM,
                                       G_SOCKET_PROTOCOL_DEFAULT,
                                       NULL);

  for (guint port = 35830; port < 35840; port++)
  {
    GError         *error         = NULL;
    GSocketAddress *bound_address = g_inet_socket_address_new (any_ip, port);

    g_socket_bind (socket,
                   bound_address,
                   TRUE,
                   &error);

    g_object_unref (bound_address);

    if (error)
    {
      if (error->code != G_IO_ERROR_ADDRESS_IN_USE)
      {
        g_warning ("g_socket_bind: %s\n", error->message);
      }
      g_clear_error (&error);
    }
    else
    {
      _port = port;
      break;
    }
  }

  g_object_unref (socket);
  g_object_unref (any_ip);

  return _port;
}

// --------------------------------------------------------------------------------
gchar *WifiCode::GetNetwork ()
{
  if (_wifi_network && (_wifi_network->IsValid ()))
  {
    return g_strdup_printf ("[%s-%s-%s]",
                            _wifi_network->GetSSID (),
                            _wifi_network->GetEncryption (),
                            _wifi_network->GetPassphrase ());
  }
  else
  {
    return g_strdup ("");
  }
}

// --------------------------------------------------------------------------------
void WifiCode::ResetKey ()
{
  g_free (_key);
  _key = NULL;
}

// --------------------------------------------------------------------------------
const gchar *WifiCode::GetKey ()
{
  if (_key == NULL)
  {
    _key = GetKey256 ();

    if (_player)
    {
      Player::AttributeId attr_id ("password");

      _player->SetAttributeValue (&attr_id,
                                  _key);
    }
  }

  return _key;
}

// --------------------------------------------------------------------------------
gchar *WifiCode::GetText ()
{
  gchar       *network = GetNetwork ();
  const gchar *ip      = Net::Ring::_broker->GetIpV4Address ();
  const gchar *key     = GetKey ();
  gchar       *text;

#ifndef LIVE_POOL
  if (_player)
  {
    gchar *name = _player->GetName ();

    text = g_strdup_printf ("%s%s:%d-%s-%u-%s",
                            network,
                            ip,
                            _port,
                            name,
                            _player->GetRef (),
                            key);
    g_free (name);
  }
  else
#endif
  {
    text = g_strdup_printf ("%s%s:%d-%s-0-%s",
                            network,
                            ip,
                            _port,
                            _text,
                            key);
  }

  g_free (network);

  return text;
}
