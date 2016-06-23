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
#ifdef WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <iphlpapi.h>
#else
  #include <ifaddrs.h>
  #include <sys/socket.h>
  #include <sys/ioctl.h>
  #include <net/if.h>
  #include <netdb.h>
#endif
#include <qrencode.h>

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
gchar *WifiCode::GetIpAddress ()
{
  gchar *ip_address = NULL;

#ifdef WIN32
  ULONG            info_length  = sizeof (IP_ADAPTER_INFO);
  PIP_ADAPTER_INFO adapter_info = (IP_ADAPTER_INFO *) malloc (sizeof (IP_ADAPTER_INFO));

  if (adapter_info)
  {
    if (GetAdaptersInfo (adapter_info, &info_length) == ERROR_BUFFER_OVERFLOW)
    {
      free (adapter_info);

      adapter_info = (IP_ADAPTER_INFO *) malloc (info_length);
    }

    if (GetAdaptersInfo (adapter_info, &info_length) == NO_ERROR)
    {
      PIP_ADAPTER_INFO adapter = adapter_info;

      while (adapter)
      {
        if (g_strcmp0 (adapter->IpAddressList.IpAddress.String, "0.0.0.0") != 0)
        {
          ip_address = g_strdup (adapter->IpAddressList.IpAddress.String);
          break;
        }

        adapter = adapter->Next;
      }
    }

    free (adapter_info);
  }
#else
    struct ifaddrs *ifa_list;

    if (getifaddrs (&ifa_list) == -1)
    {
      g_error ("getifaddrs");
    }
    else
    {
      for (struct ifaddrs *ifa = ifa_list; ifa != NULL; ifa = ifa->ifa_next)
      {
        if (   ifa->ifa_addr
            && (ifa->ifa_flags & IFF_UP)
            && ((ifa->ifa_flags & IFF_LOOPBACK) == 0))
        {
          int family = ifa->ifa_addr->sa_family;

          if (family == AF_INET)
          {
            char host[NI_MAXHOST];

            if (getnameinfo (ifa->ifa_addr,
                             sizeof (struct sockaddr_in),
                             host,
                             NI_MAXHOST, NULL, 0, NI_NUMERICHOST) == 0)
            {
              ip_address = g_strdup (host);
              break;
            }
          }
        }
      }

      freeifaddrs (ifa_list);
    }
#endif

  return ip_address;
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
  gchar *network = GetNetwork ();
  gchar *ip      = GetIpAddress ();
  gchar *key     = GetKey ();
  gchar *text;

#ifndef LIVE_POOL
  if (_player)
  {
    text = g_strdup_printf ("%s%s:%d-%s-%d-%s",
                            network,
                            ip,
                            _port,
                            _player->GetName (),
                            _player->GetRef (),
                            key);
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
  g_free (ip);
  g_free (network);

  return text;
}
