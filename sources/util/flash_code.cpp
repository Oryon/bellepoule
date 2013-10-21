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
#include "flash_code.hpp"

// --------------------------------------------------------------------------------
FlashCode::FlashCode (Player *player)
  : Object ("FlashCode")
{
  _pixbuf = NULL;
  _player = player;
}

// --------------------------------------------------------------------------------
FlashCode::~FlashCode ()
{
  g_object_unref (_pixbuf);
}

// --------------------------------------------------------------------------------
void FlashCode::DestroyPixbuf (guchar   *pixels,
                               gpointer  data)
{
  g_free (pixels);
}

// --------------------------------------------------------------------------------
gchar *FlashCode::GetIpAddress ()
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
gchar *FlashCode::GetKey ()
{
#if 0
  gchar *key;
  GHmac *hmac = g_hmac_new (G_CHECKSUM_SHA1,
                            (const guchar *) this,
                            50);

  key = g_strdup (g_hmac_get_string (hmac));
  g_hmac_unref (hmac);

  return key;
#endif

  return g_strdup ("123456");
}

// --------------------------------------------------------------------------------
GdkPixbuf *FlashCode::GetPixbuf ()
{
  if (_pixbuf == NULL)
  {
    QRcode *qr_code;

    {
      gchar *ip   = GetIpAddress ();
      gchar *key  = GetKey ();
      gchar *code = g_strdup_printf ("%s:35830-%s-%d-%s", ip, _player->GetName (), _player->GetRef (), key);

      qr_code = QRcode_encodeString (code,
                                     0,
                                     QR_ECLEVEL_L,
                                     QR_MODE_8,
                                     1);
      g_free (code);
      g_free (key);
      g_free (ip);
    }

    {
      GdkPixbuf *small_pixbuf;
      guint      stride = qr_code->width * 3;
      guchar    *data   = g_new (guchar, qr_code->width *stride);

      for (gint y = 0; y < qr_code->width; y++)
      {
        guchar *row = qr_code->data + (y * qr_code->width);
        guchar *p   = data + (y * stride);

        for (gint x = 0; x < qr_code->width; x++)
        {
          if (row[x] & 0x1)
          {
            p[0] = 0x0;
            p[1] = 0x0;
            p[2] = 0x0;
          }
          else
          {
            p[0] = 0xFF;
            p[1] = 0xFF;
            p[2] = 0xFF;
          }
          p += 3;
        }
      }

      small_pixbuf = gdk_pixbuf_new_from_data (data,
                                               GDK_COLORSPACE_RGB,
                                               FALSE,
                                               8,
                                               qr_code->width,
                                               qr_code->width,
                                               stride,
                                               DestroyPixbuf,
                                               NULL);

      _pixbuf = gdk_pixbuf_scale_simple (small_pixbuf,
                                         qr_code->width * 3,
                                         qr_code->width * 3,
                                         GDK_INTERP_NEAREST);
      g_object_unref (small_pixbuf);
    }
  }

  return _pixbuf;
}
