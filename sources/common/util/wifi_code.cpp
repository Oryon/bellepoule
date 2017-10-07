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

#include "network/ring.hpp"

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
void WifiCode::SetIpPort (guint port)
{
  _port = port;
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
gchar *WifiCode::GetKey ()
{
  if (_key == NULL)
  {
    static const guint32  data_length = 256 / 8;
    GRand                *random      = g_rand_new ();

    _key = g_new (gchar, data_length+1);
    for (guint i = 0; i < data_length; i++)
    {
      guint ascii_set = g_rand_int_range (random, 0, 3);

      if (ascii_set == 0)
      {
        _key[i] = g_rand_int_range (random,
                                    '0',
                                    '9');
      }
      else if (ascii_set == 1)
      {
        _key[i] = g_rand_int_range (random,
                                    'A',
                                    'Z');
      }
      else
      {
        _key[i] = g_rand_int_range (random,
                                    'a',
                                    'z');
      }
    }
    _key[data_length] = '\0';

    g_rand_free (random);
  }

  return g_strdup ((gchar *) _key);
}

// --------------------------------------------------------------------------------
gchar *WifiCode::GetText ()
{
  gchar       *network = GetNetwork ();
  const gchar *ip      = Net::Ring::_broker->GetIpV4Address ();
  gchar       *key     = GetKey ();
  gchar       *text;

#ifndef LIVE_POOL
  if (_player)
  {
    gchar *name = _player->GetName ();

    text = g_strdup_printf ("%s%s:%d-%s-%d-%s",
                            network,
                            ip,
                            _port,
                            _player->GetName (),
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

  g_free (key);
  g_free (network);

  return text;
}
