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

#include <errno.h>

#include <gio/gnetworking.h>
#ifndef WIN32
  #include <ifaddrs.h>
#endif

#include "util/object.hpp"
#include "partner.hpp"
#include "message.hpp"
#include "ring.hpp"

namespace Net
{
  const gchar *Ring::ANNOUNCE_GROUP = "225.0.0.35";

  gchar          *Ring::_role              = NULL;
  gchar          *Ring::_ip_address        = NULL;
  guint           Ring::_unicast_port      = 0;
  GList          *Ring::_partner_list      = NULL;
  GList          *Ring::_message_list      = NULL;
  GtkWidget      *Ring::_partner_indicator = NULL;
  GList          *Ring::_listeners         = NULL;
  GSocketAddress *Ring::_multicast_address = NULL;

  // --------------------------------------------------------------------------------
  Ring::Ring ()
  {
  }

  // --------------------------------------------------------------------------------
  Ring::~Ring ()
  {
  }

  // --------------------------------------------------------------------------------
  const gchar *Ring::GetRole ()
  {
    return _role;
  }

  // --------------------------------------------------------------------------------
  void Ring::Join (const gchar *role,
                   guint        unicast_port,
                   GtkWidget   *partner_indicator)
  {
    if (_role == NULL)
    {
      _role = g_strdup (role);

      _unicast_port = unicast_port;

      _partner_indicator = partner_indicator;
      if (_partner_indicator)
      {
        gtk_widget_set_sensitive (_partner_indicator, FALSE);
        gtk_widget_set_tooltip_text (_partner_indicator, "");
      }

      // Multicast listener
      {
        GError  *error  = NULL;
        GSocket *socket;

        socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
                               G_SOCKET_TYPE_DATAGRAM,
                               G_SOCKET_PROTOCOL_DEFAULT,
                               NULL);

        {
          GInetAddress   *any_ip        = g_inet_address_new_any    (G_SOCKET_FAMILY_IPV4);
          GSocketAddress *bound_address = g_inet_socket_address_new (any_ip, ANNOUNCE_PORT);

          g_socket_bind (socket,
                         bound_address,
                         TRUE,
                         &error);

          g_object_unref (any_ip);
          g_object_unref (bound_address);
        }

        if (error)
        {
          g_warning ("g_socket_bind: %s\n", error->message);
          g_clear_error (&error);
        }
        else if (JoinMulticast (socket))
        {
          GSource *source = g_socket_create_source (socket,
                                                    (GIOCondition) (G_IO_IN|G_IO_ERR|G_IO_HUP),
                                                    NULL);
          g_source_set_callback (source,
                                 (GSourceFunc) OnMulticast,
                                 NULL,
                                 NULL);
          g_source_attach (source, NULL);
          g_source_unref (source);
        }

        g_object_unref (socket);
      }

      AnnounceAvailability ();
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Ring::JoinMulticast (GSocket *socket)
  {
    GInetAddress *address = g_inet_address_new_from_string (ANNOUNCE_GROUP);
    GError       *error   = NULL;

    g_socket_join_multicast_group (socket,
                                   address,
                                   FALSE,
                                   NULL,
                                   &error);
    if (error == NULL)
    {
      _ip_address = GuessIpV4Address ();
    }
    else
    {
      g_clear_error (&error);

      g_object_unref (address);
      address = g_inet_address_new_loopback (G_SOCKET_FAMILY_IPV4);
      _ip_address = g_inet_address_to_string (address);
    }

    _multicast_address = g_inet_socket_address_new (address,
                                                    ANNOUNCE_PORT);
    g_object_unref (address);

    return TRUE;
  }

  // --------------------------------------------------------------------------------
  void Ring::Leave ()
  {
    {
      Message *message = new Message ("Farewell");

      message->Set ("role", _role);

      Multicast (message);
      message->Release ();
    }

    {
      g_free (_role);
      _role = NULL;
    }

    {
      g_list_free_full (_partner_list,
                        (GDestroyNotify) Object::TryToRelease);
      _partner_list = NULL;
    }
  }

  // --------------------------------------------------------------------------------
  void Ring::Handshake (Message *message)
  {
    Partner *partner = new Partner (message);

    Add (partner);
    partner->Release ();
  }

  // --------------------------------------------------------------------------------
  void Ring::AnnounceAvailability ()
  {
    Message *message = new Message ("Announcement");

    message->Set ("role",         _role);
    message->Set ("ip_address",   _ip_address);
    message->Set ("unicast_port", _unicast_port);

    Multicast (message);
    message->Release ();
  }

  // --------------------------------------------------------------------------------
  void Ring::Multicast (Message *message)
  {
    GError  *error  = NULL;
    GSocket *socket;

    socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
                           G_SOCKET_TYPE_DATAGRAM,
                           G_SOCKET_PROTOCOL_DEFAULT,
                           &error);
    if (error)
    {
      g_warning ("g_socket_new: %s\n", error->message);
      g_clear_error (&error);
    }
    else
    {
      gchar *parcel = message->GetParcel ();

      g_socket_set_blocking (socket, FALSE);

      g_socket_send_to (socket,
                        _multicast_address,
                        parcel,
                        strlen (parcel) + 1,
                        NULL,
                        &error);
      if (error)
      {
        g_warning ("sendto: %s", error->message);
        g_clear_error (&error);
      }

      g_free (parcel);
    }
  }

  // -------------------------------------------------------------------------------
  gboolean Ring::OnMulticast (GSocket      *socket,
                              GIOCondition  condition,
                              gpointer      user_data)
  {
    if (condition == G_IO_HUP)
    {
      g_warning ("Multicast error: G_IO_HUP\n");
    }
    if (condition == G_IO_ERR)
    {
      g_warning ("Multicast error: G_IO_ERR\n");
    }
    else if (condition == G_IO_IN)
    {
      GError *error       = NULL;
      gchar   buffer[500];

      g_socket_receive (socket,
                        buffer,
                        sizeof (buffer),
                        NULL,
                        &error);
      if (error)
      {
        g_warning ("g_socket_receive: %s", error->message);
        g_clear_error (&error);
      }
      else
      {
        Message *message = new Message ((guint8 *) buffer);
        gchar   *role    = message->GetString ("role");

        if (g_strcmp0 (role, _role) != 0)
        {
          if (message->Is ("Announcement"))
          {
            Remove (role);

            {
              Partner *partner = new Partner (message);

              Add (partner);
              partner->Release ();
            }
          }
          else if (message->Is ("Farewell"))
          {
            Remove (role);
          }
        }

        g_free (role);
        message->Release ();
      }
    }

    return G_SOURCE_CONTINUE;
  }

  // -------------------------------------------------------------------------------
  void Ring::Add (Partner *partner)
  {
    {
      GList *current = _partner_list;

      while (current)
      {
        if (partner->Is ((Partner *) current->data))
        {
          return;
        }

        current = g_list_next (current);
      }
    }

    if (_partner_indicator)
    {
      gtk_widget_set_sensitive (_partner_indicator, TRUE);
      gtk_widget_set_tooltip_markup (_partner_indicator,
                                     partner->GetAddress ());
    }

    {
      Message *message = new Message ("Handshake");

      message->Set ("role",         _role);
      message->Set ("ip_address",   _ip_address);
      message->Set ("unicast_port", _unicast_port);
      partner->SendMessage (message);
      message->Release ();
    }

    partner->Retain ();
    _partner_list = g_list_prepend (_partner_list,
                                    partner);
    Synchronize (partner);
  }

  // -------------------------------------------------------------------------------
  void Ring::Remove (const gchar *role)
  {
    GList *current = _partner_list;

    while (current)
    {
      Partner *partner = (Partner *) current->data;

      if (partner->HasRole (role))
      {
        if (_partner_indicator)
        {
          gtk_widget_set_sensitive (_partner_indicator, FALSE);
          gtk_widget_set_tooltip_text (_partner_indicator, "");
        }

        _partner_list = g_list_delete_link (_partner_list,
                                            current);
        partner->Release ();
        break;
      }

      current = g_list_next (current);
    }
  }

  // -------------------------------------------------------------------------------
  void Ring::Synchronize (Partner *partner)
  {
    GList *current = g_list_last (_message_list);

    while (current)
    {
      Message *message = (Message *) current->data;

      partner->SendMessage (message);
      current = g_list_previous (current);
    }
  }

  // -------------------------------------------------------------------------------
  void Ring::Send (Message *message)
  {
    GList *current = _partner_list;

    while (current)
    {
      Partner *partner = (Partner *) current->data;

      partner->SendMessage (message);
      current = g_list_next (current);
    }
  }

  // -------------------------------------------------------------------------------
  void Ring::SpreadMessage (Message *message)
  {
    if (g_list_find (_message_list,
                     message) == NULL)
    {
      _message_list = g_list_prepend (_message_list,
                                      message);
    }

    message->SetFitness (1);
    Send (message);
  }

  // -------------------------------------------------------------------------------
  void Ring::RecallMessage (Message *message)
  {
    GList *node = g_list_find (_message_list,
                               message);

    if (node)
    {
      message->SetFitness (0);
      Send (message);

      _message_list = g_list_delete_link (_message_list,
                                          node);
    }
  }

  // -------------------------------------------------------------------------------
  void Ring::RegisterListener (Listener *listener)
  {
    _listeners = g_list_prepend (_listeners,
                                 listener);
  }

  // -------------------------------------------------------------------------------
  void Ring::UnregisterListener (Listener *listener)
  {
    GList *node = g_list_find (_listeners,
                               listener);

    if (node)
    {
      _listeners = g_list_delete_link (_listeners,
                                       node);
    }
  }

  // -------------------------------------------------------------------------------
  void Ring::PostToListener (Net::Message *message)
  {
    GList *current = _listeners;

    while (current)
    {
      Listener *listener = (Listener *) current->data;

      listener->OnMessage (message);
      current = g_list_next (current);
    }
  }

  // --------------------------------------------------------------------------------
  const gchar *Ring::GetIpV4Address ()
  {
    return _ip_address;
  }

  // --------------------------------------------------------------------------------
  gchar *Ring::GuessIpV4Address ()
  {
    gchar *ip_address = NULL;

#ifdef WIN32
#if 1
    {
      struct hostent *hostinfo;
      gchar           hostname[50];

      if (gethostname (hostname, sizeof (hostname)) == 0)
      {
        hostinfo = gethostbyname (hostname);
        if (hostinfo)
        {
          ip_address = g_strdup (inet_ntoa (*(struct in_addr*) (hostinfo->h_addr)));
        }
      }
    }
#else
    {
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
    }
#endif
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
}
