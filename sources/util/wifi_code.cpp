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

#include "common/player.hpp"
#include "wifi_code.hpp"

Net::WifiNetwork *WifiCode::_wifi_network = NULL;

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

  if (_player)
  {
    _player->Retain ();
  }
}

// --------------------------------------------------------------------------------
WifiCode::~WifiCode ()
{
  g_free (_key);
  _player->Release ();
}

// --------------------------------------------------------------------------------
void WifiCode::SetWifiNetwork (Net::WifiNetwork *network)
{
  _wifi_network = network;
}

// --------------------------------------------------------------------------------
gchar *WifiCode::GetNetwork ()
{
  if (_wifi_network->IsValid ())
  {
    return g_strdup_printf ("[%s-%s-%s]",
                            _wifi_network->GetSSID (),
                            _wifi_network->GetEncryption (),
                            _wifi_network->GetPassphrase ());
  }
  else
  {
    return g_strdup_printf ("");
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
        if (strcmp (adapter->IpAddressList.IpAddress.String, "0.0.0.0") != 0)
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
gchar *WifiCode::GetKey ()
{
  if (_key == NULL)
  {
    static const guint32  data_length = 50;
    GRand                *random      = g_rand_new ();
    guchar               *data        = g_new (guchar, data_length);

    for (guint i = 0; i < data_length; i++)
    {
      data[i] = g_rand_int (random);
    }

    _key = g_compute_checksum_for_data (G_CHECKSUM_SHA1,
                                        data,
                                        data_length);

    g_rand_free (random);
    g_free (data);
  }

  return g_strdup (_key);
}

// --------------------------------------------------------------------------------
gchar *WifiCode::GetText ()
{
  gchar *network = GetNetwork ();
  gchar *ip      = GetIpAddress ();
  gchar *key     = GetKey ();
  gchar *text;

  if (_player)
  {
    text = g_strdup_printf ("%s%s:35830-%s-%d-%s",
                            network,
                            ip,
                            _player->GetName (),
                            _player->GetRef (),
                            key);
  }
  else
  {
    text = g_strdup_printf ("%s%s:35830-%s-0-%s",
                            network,
                            ip,
                            _text,
                            key);
  }

  g_free (key);
  g_free (ip);
  g_free (network);

  return text;
}
