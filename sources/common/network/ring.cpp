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

#include <string.h>
#include <errno.h>

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "util/object.hpp"
#include "partner.hpp"
#include "message.hpp"
#include "ring.hpp"

namespace Net
{
  const gchar *Ring::ANNOUNCE_GROUP = "225.0.0.35";

  gchar     *Ring::_role              = NULL;
  guint      Ring::_unicast_port      = 0;
  GList     *Ring::_partner_list      = NULL;
  GList     *Ring::_message_list      = NULL;
  GtkWidget *Ring::_partner_indicator = NULL;

  // --------------------------------------------------------------------------------
  Ring::Ring ()
  {
  }

  // --------------------------------------------------------------------------------
  Ring::~Ring ()
  {
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
    message->Set ("unicast_port", _unicast_port);

    Multicast (message);
    message->Release ();
  }

  // --------------------------------------------------------------------------------
  void Ring::Multicast (Message *message)
  {
    struct sockaddr_in  addr;
    int                 fd;

    if ((fd = socket (AF_INET,
                      SOCK_DGRAM,
                      0)) < 0)
    {
      g_warning ("socket: %s", strerror (errno));
      return;
    }

    {
      memset (&addr,
              0,
              sizeof(addr));

      addr.sin_family      = AF_INET;
      addr.sin_addr.s_addr = inet_addr (ANNOUNCE_GROUP);
      addr.sin_port        = htons     (ANNOUNCE_PORT);
    }

    {
      gchar *parcel = message->GetParcel ();

      if (sendto (fd,
                  parcel,
                  strlen (parcel) + 1,
                  0,
                  (struct sockaddr *) &addr,
                  sizeof(addr)) < 0)
      {
        g_warning ("sendto: %s", strerror (errno));
      }

      g_free (parcel);
    }
  }

  // -------------------------------------------------------------------------------
  gboolean Ring::OnMulticast (Message *message)
  {
    gchar *role = message->GetString ("role");

    if (message->Is ("Announcement"))
    {
      if (strcmp (role, _role) != 0)
      {
        Add (new Partner (message));
      }
    }
    else if (message->Is ("Farewell"))
    {
      GList *node = g_list_find_custom (_partner_list,
                                        role,
                                        (GCompareFunc) Partner::CompareRole);

      if (node)
      {
        if (_partner_indicator)
        {
          gtk_widget_set_sensitive (_partner_indicator, FALSE);
        }

        _partner_list = g_list_delete_link (_partner_list,
                                            node);
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
      guint8             buffer[100];
      ssize_t            size;

      if ((size = recvfrom (fd,
                            buffer,
                            100,
                            0,
                            (struct sockaddr *) &from,
                            &addrlen)) < 0)
      {
        g_warning ("recvfrom: %s", strerror (errno));
      }

      {
        Message *message = new Message (buffer,
                                        &from);

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
        if (partner->HasSameRole ((Partner *) current->data))
        {
          return;
        }

        current = g_list_next (current);
      }
    }

    if (_partner_indicator)
    {
      gtk_widget_set_sensitive (_partner_indicator, TRUE);
    }

    {
      Message *message = new Message ("Handshake");

      message->Set ("role",         _role);
      message->Set ("unicast_port", _unicast_port);
      partner->SendMessage (message);
      message->Release ();
    }

    _partner_list = g_list_prepend (_partner_list,
                                    partner);
    Synchronize (partner);
  }

  // -------------------------------------------------------------------------------
  void Ring::Synchronize (Partner *partner)
  {
    GList *current = _message_list;

    while (current)
    {
      Message *message = (Message *) current->data;

      partner->SendMessage (message);
      current = g_list_next (current);
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
    message->SetFitness (0);
    Send (message);

    {
      GList *node = g_list_find (_message_list,
                                 message);

      if (node)
      {
        _message_list = g_list_delete_link (_message_list,
                                            node);
      }
    }
  }
}
