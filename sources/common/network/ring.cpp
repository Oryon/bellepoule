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

#include "util/object.hpp"
#include "http_server.hpp" // !!
#include "partner.hpp"
#include "message.hpp"
#include "ring.hpp"

namespace Net
{
  const gchar *Ring::ANNOUNCE_GROUP = "225.0.0.35";

  gchar     *Ring::_role              = NULL;
  gchar     *Ring::_ip_address        = NULL;
  guint      Ring::_unicast_port      = 0;
  GList     *Ring::_partner_list      = NULL;
  GList     *Ring::_message_list      = NULL;
  GtkWidget *Ring::_partner_indicator = NULL;
  GList     *Ring::_listeners         = NULL;

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

      _ip_address   = HttpServer::GetIpV4 ();
      _unicast_port = unicast_port;

      _partner_indicator = partner_indicator;
      if (_partner_indicator)
      {
        gtk_widget_set_sensitive (_partner_indicator, FALSE);
        gtk_widget_set_tooltip_text (_partner_indicator, "");
      }

      // Listen to partner announcement
      {
        GError *error = NULL;

        if (!g_thread_create ((GThreadFunc) MulticastListener,
                              0,
                              FALSE,
                              &error))
        {
          g_warning ("Failed to create MulticastListener thread: %s\n", error->message);
          g_error_free (error);
        }
      }

      AnnounceAvailability ();
    }
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
    Add (new Partner (message));
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
      GSocketAddress *socket_address;

      {
        GInetAddress *inet_address = g_inet_address_new_from_string (ANNOUNCE_GROUP);

        socket_address = g_inet_socket_address_new (inet_address,
                                                    ANNOUNCE_PORT);
        g_object_unref (inet_address);
      }

      {
        gchar *parcel = message->GetParcel ();

        g_socket_send_to (socket,
                          socket_address,
                          parcel,
                          strlen (parcel) + 1,
                          NULL,
                          &error);
        if (error)
        {
          g_warning ("sendto: %s", strerror (errno));
          g_clear_error (&error);
        }

        g_free (parcel);
      }

      g_object_unref (socket_address);
    }
  }

  // -------------------------------------------------------------------------------
  gboolean Ring::OnMulticast (Message *message)
  {
    gchar *role = message->GetString ("role");

    if (g_strcmp0 (role, _role) != 0)
    {
      if (message->Is ("Announcement"))
      {
        Remove (role);
        Add (new Partner (message));
      }
      else if (message->Is ("Farewell"))
      {
        Remove (role);
      }
    }

    g_free (role);
    message->Release ();

    return FALSE;
  }

  // -------------------------------------------------------------------------------
  gpointer Ring::MulticastListener ()
  {
    struct sockaddr_in addr;
    int                fd;

    if ((fd = socket (AF_INET,
                      SOCK_DGRAM,
                      0)) < 0)
    {
      g_warning ("socket: %s", strerror (errno));
      return NULL;
    }

    // allow multiple sockets to use the same PORT number
    {
      guint yes = 1;

      if (setsockopt (fd,
                      SOL_SOCKET,
                      SO_REUSEADDR,
                      (const char *) &yes,
                      sizeof (yes)) < 0)
      {
        g_warning ("setsockopt: %s", strerror (errno));
        return NULL;
      }
    }

    {
      memset (&addr,
              0,
              sizeof(addr));

      addr.sin_family      = AF_INET;
      addr.sin_addr.s_addr = htonl (INADDR_ANY);
      addr.sin_port        = htons (ANNOUNCE_PORT);
    }

    if (bind (fd,
              (struct sockaddr *) &addr,
              sizeof (addr)) < 0)
    {
      g_warning ("socket: %s", strerror (errno));
      return NULL;
    }

    // use setsockopt() to request that the kernel join a multicast group
    {
      struct ip_mreq option;

      option.imr_multiaddr.s_addr = inet_addr (ANNOUNCE_GROUP);
      option.imr_interface.s_addr = htonl     (INADDR_ANY);
      if (setsockopt (fd,
                      IPPROTO_IP,
                      IP_ADD_MEMBERSHIP,
                      (const char *) &option,
                      sizeof (option)) < 0)
      {
        g_warning ("setsockopt: %s", strerror (errno));
        return NULL;
      }
    }

    while (1)
    {
      struct sockaddr_in from;
      socklen_t          addrlen = sizeof (from);
      gchar              buffer[500];
      ssize_t            size;

      if ((size = recvfrom (fd,
                            buffer,
                            sizeof (buffer),
                            0,
                            (struct sockaddr *) &from,
                            &addrlen)) < 0)
      {
        g_warning ("recvfrom: %s", strerror (errno));
      }

      {
        Message *message = new Message ((guint8 *) buffer);

        g_idle_add ((GSourceFunc) OnMulticast,
                    message);
      }
    }

    return NULL;
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
}
