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
#include "util/global.hpp"
#include "util/user_config.hpp"
#include "credentials.hpp"
#include "message.hpp"
#include "cryptor.hpp"
#include "ring.hpp"

namespace Net
{
  const gchar *Ring::ANNOUNCE_GROUP = "225.0.0.35";
  const gchar *Ring::SECRET         = "tagada soinsoin";

  Ring *Ring::_broker = NULL;

  // --------------------------------------------------------------------------------
  Ring::Ring (Role       role,
              guint      unicast_port,
              GtkWidget *partner_indicator)
    : Object ("Ring")
  {
    _ip_address         = NULL;
    _partner_list       = NULL;
    _message_list       = NULL;
    _listeners          = NULL;
    _handshake_listener = NULL;
    _multicast_address  = NULL;
    _role               = role;
    _unicast_port       = unicast_port;
    _credentials        = NULL;
    _partner_id         = g_random_int ();

    _partner_indicator = partner_indicator;

    {
      gchar *passphrase = g_key_file_get_string (Global::_user_config->_key_file,
                                                 "Ring", "passphrase",
                                                 NULL);

      if (passphrase == NULL)
      {
        passphrase = FlashCode::GetKey256 ();
      }

      ChangePassphrase (passphrase);
      g_free (passphrase);
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
                               this,
                               NULL);
        g_source_attach (source, NULL);
        g_source_unref (source);
      }

      g_object_unref (socket);
    }

    _heartbeat_timer = g_timeout_add_seconds (5,
                                              (GSourceFunc) SendHeartbeat,
                                              this);
  }

  // --------------------------------------------------------------------------------
  Ring::~Ring ()
  {
    if (_heartbeat_timer)
    {
      g_source_remove (_heartbeat_timer);
    }

    _credentials->Release ();
  }

  // --------------------------------------------------------------------------------
  void Ring::DisplayIndicator ()
  {
    if (_partner_indicator)
    {
      gchar   *icon;
      GString *tooltip = g_string_new ("");

      for (GList *current = _partner_list; current; current = g_list_next (current))
      {
        Partner *partner = (Partner *) current->data;

        if (current != _partner_list)
        {
          g_string_append_c (tooltip, '\n');
        }

        g_string_append_printf (tooltip, "%s:<b>%d</b>",
                                partner->GetAddress (),
                                partner->GetPort ());
      }

      if (_partner_list)
      {
        icon = g_build_filename (Global::_share_dir, "resources", "glade", "images", "network.png", NULL);
        gtk_image_menu_item_get_image (GTK_IMAGE_MENU_ITEM (_partner_indicator));
        gtk_widget_set_tooltip_markup (_partner_indicator,
                                       tooltip->str);
      }
      else
      {
        icon = g_build_filename (Global::_share_dir, "resources", "glade", "images", "network-ko.png", NULL);
        gtk_widget_set_tooltip_text (_partner_indicator, "");
      }

      {
        GtkWidget *image = gtk_image_menu_item_get_image (GTK_IMAGE_MENU_ITEM (_partner_indicator));

        gtk_image_set_from_file (GTK_IMAGE (image),
                                 icon);
      }

      g_string_free (tooltip,
                     TRUE);
      g_free (icon);
    }
  }

  // --------------------------------------------------------------------------------
  gchar *Ring::GetCryptorKey ()
  {
    return _credentials->GetKey ();
  }

  // --------------------------------------------------------------------------------
  void Ring::Join (Role       role,
                   guint      unicast_port,
                   GtkWidget *partner_indicator)
  {
    if (_broker)
    {
      g_warning ("Ring instanciated more than once");
    }

    _broker = new Ring (role,
                        unicast_port,
                        partner_indicator);

    _broker->AnnounceAvailability ();
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

      message->Set ("partner", _partner_id);
      message->Set ("role",    _role);

      Multicast (message);
      message->Release ();
    }

    FreeFullGList (Partner, _partner_list);
  }

  // --------------------------------------------------------------------------------
  void Ring::OnHandshake (Message           *message,
                          HandshakeListener *listener)
  {
    HandshakeResult response = (HandshakeResult) message->GetInteger ("response");

    _handshake_listener = listener;

    if (response == GRANTED)
    {
      Partner *partner = new Partner (message,
                                      this);

      if (Add (partner) == FALSE)
      {
        g_error ("Ring::OnHandshake");
      }

      partner->Release ();
    }

    _handshake_listener->OnHanshakeResult (response);
  }

  // --------------------------------------------------------------------------------
  void Ring::AnnounceAvailability ()
  {
    Message *message = new Message ("Announcement");

    message->Set ("role",         _role);
    message->Set ("ip_address",   _ip_address);
    message->Set ("unicast_port", _unicast_port);
    message->Set ("partner",      _partner_id);

    // Add secret challenge to message
    {
      gchar   *iv;
      char    *crypted;
      char    *key     = _credentials->GetKey ();
      Cryptor *cryptor = new Cryptor ();

      crypted = cryptor->Encrypt (SECRET,
                                  key,
                                  &iv);
      message->Set ("iv",     iv);
      message->Set ("secret", crypted);

      g_free (iv);
      g_free (crypted);
      g_free (key);

      cryptor->Release ();
    }

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
                              Ring         *ring)
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
        gint32   id      = (Role) message->GetSignedInteger ("partner");

        if (ring->_partner_id != id)
        {
          if (message->Is ("Announcement"))
          {
            Role     role    = (Role) message->GetInteger ("role");
            Partner *partner = new Partner (message, ring);

            if ((ring->_role == role) && (role == RESOURCE_MANAGER))
            {
              ring->SendHandshake (partner,
                                   ROLE_REJECTED);
            }
            else if (role != ring->_role)
            {
              if (ring->DecryptSecret (message))
              {
                if (ring->Add (partner))
                {
                  ring->SendHandshake (partner,
                                       GRANTED);
                }
                else
                {
                  ring->SendHandshake (partner,
                                       ROLE_REJECTED);
                }
              }
              else
              {
                ring->SendHandshake (partner,
                                     AUTHENTICATION_FAILED);
              }
            }
            partner->Release ();
          }
          else if (message->Is ("Farewell"))
          {
            for (GList *current = ring->_partner_list; current; current = g_list_next (current))
            {
              Partner *partner = (Partner *) current->data;

              if (partner->Is (message->GetSignedInteger ("partner")))
              {
                ring->Remove (partner);
                break;
              }
            }
          }
        }
        message->Release ();
      }
    }

    return G_SOURCE_CONTINUE;
  }

  // -------------------------------------------------------------------------------
  void Ring::SendHandshake (Partner         *partner,
                            HandshakeResult  result)
  {
    Message *message = new Message ("Handshake");

    if (result == GRANTED)
    {
      message->Set ("role",         _role);
      message->Set ("ip_address",   _ip_address);
      message->Set ("unicast_port", _unicast_port);
      message->Set ("partner",      _partner_id);
    }

    message->Set ("response", result);

    message->SetFitness (1);
    partner->SendMessage (message);
    message->Release ();
  }

  // -------------------------------------------------------------------------------
  gboolean Ring::Add (Partner *partner)
  {
    if (partner->HasRole (RESOURCE_MANAGER))
    {
      for (GList *current = _partner_list; current; current = g_list_next (current))
      {
        Partner *p = (Partner *) current->data;

        if (p->HasRole (RESOURCE_MANAGER))
        {
          return FALSE;
        }
      }
    }

    {
      char *key = _credentials->GetKey ();

      partner->SetPassPhrase256 (key);
      g_free (key);
    }

    partner->Retain ();
    _partner_list = g_list_prepend (_partner_list,
                                    partner);
    NotifyPartnerStatus (partner,
                         TRUE);
    Synchronize (partner);

    DisplayIndicator ();

    return TRUE;
  }

  // -------------------------------------------------------------------------------
  void Ring::Remove (Partner *partner)
  {
    GList *node = g_list_find (_partner_list,
                               partner);

    if (node)
    {
      _partner_list = g_list_delete_link (_partner_list,
                                          node);
      NotifyPartnerStatus (partner,
                           FALSE);
      partner->Release ();

      DisplayIndicator ();
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
    for (GList *current = _partner_list; current; current = g_list_next (current))
    {
      Partner *partner = (Partner *) current->data;

      partner->SendMessage (message);
    }
  }

  // -------------------------------------------------------------------------------
  void Ring::OnObjectDeleted (Object *object)
  {
    Message *message = (Message *)object;
    GList   *node    = g_list_find (_message_list,
                                    message);

    if (node)
    {
      _message_list = g_list_delete_link (_message_list,
                                          node);
    }
  }

  // -------------------------------------------------------------------------------
  void Ring::SpreadMessage (Message *message)
  {
    if (g_list_find (_message_list,
                     message) == NULL)
    {
      message->AddObjectListener (this);
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
      _message_list = g_list_delete_link (_message_list,
                                          node);
    }

    if (message->GetFitness ())
    {
      message->SetFitness (0);
      Send (message);
    }
  }

  // -------------------------------------------------------------------------------
  void Ring::RegisterListener (Listener *listener)
  {
    if (g_list_find (_listeners,
                     listener) == NULL)
    {
      _listeners = g_list_prepend (_listeners,
                                   listener);

      for (GList *current = _partner_list; current; current = g_list_next (current))
      {
        Partner *partner = (Partner *) current->data;

        NotifyPartnerStatus (partner,
                             TRUE);
      }
    }
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
  Partner *Ring::GetPartner ()
  {
    g_warning ("Ring::GetPartner: STUB");
    if (_partner_list)
    {
      return (Partner *) _partner_list->data;
    }
    return NULL;
  }

  // -------------------------------------------------------------------------------
  void Ring::NotifyPartnerStatus (Partner  *partner,
                                  gboolean  joined)
  {
    GList *current = _listeners;

    while (current)
    {
      Listener *listener = (Listener *) current->data;

      listener->OnPartnerJoined (partner,
                                 joined);

      current = g_list_next (current);
    }
  }

  // --------------------------------------------------------------------------------
  void Ring::ChangePassphrase (const gchar *passphrase)
  {
    gchar *ip_address = GuessIpV4Address ();

    g_key_file_set_string (Global::_user_config->_key_file,
                           "Ring", "passphrase", passphrase);

    Object::TryToRelease (_credentials);
    _credentials = new Credentials (GetRoleImage (),
                                    ip_address,
                                    _unicast_port,
                                    passphrase);

    g_free (ip_address);

    Global::_user_config->Save ();
  }

  // --------------------------------------------------------------------------------
  FlashCode *Ring::GetFlashCode ()
  {
    return _credentials;
  }

  // --------------------------------------------------------------------------------
  gboolean Ring::DecryptSecret (Message *message)
  {
    gboolean decoded = FALSE;
    gchar   *crypted = message->GetString ("secret");
    gchar   *iv      = message->GetString ("iv");

    if (crypted && iv)
    {
      decoded = DecryptSecret (crypted,
                               iv,
                               _credentials);

      if (decoded == FALSE)
      {
        UserConfig *partner_config;
        gchar      *partner_pass;

        if (_role == RESOURCE_USER)
        {
          partner_config = new UserConfig ("BellePoule2D", TRUE);
        }
        else
        {
          partner_config = new UserConfig ("BellePoule", TRUE);
        }

        partner_pass = g_key_file_get_string (partner_config->_key_file,
                                              "Ring", "passphrase",
                                              NULL);

        if (partner_pass)
        {
          gchar       *ip_address           = GuessIpV4Address ();
          Credentials *partner_crendentials = new Credentials (GetRoleImage (),
                                                               ip_address,
                                                               _unicast_port,
                                                               partner_pass);

          decoded = DecryptSecret (crypted,
                                   iv,
                                   partner_crendentials);

          if (decoded)
          {
            ChangePassphrase (partner_pass);
          }

          partner_crendentials->Release ();
          g_free (partner_pass);
          g_free (ip_address);
        }

        partner_config->Release ();
      }
    }
    g_free (crypted);
    g_free (iv);

    return decoded;
  }

  // --------------------------------------------------------------------------------
  gboolean Ring::DecryptSecret (gchar       *crypted,
                                gchar       *iv,
                                Credentials *credentials)
  {
    gboolean decoded = FALSE;
    Cryptor *cryptor = new Cryptor ();
    gchar   *key     = credentials->GetKey ();
    gchar   *secret;

    secret = cryptor->Decrypt (crypted,
                               iv,
                               key);

    if (g_ascii_strcasecmp (secret, SECRET) == 0)
    {
      decoded = TRUE;
    }

    g_free (key);
    g_free (secret);
    cryptor->Release ();

    return decoded;
  }

  // --------------------------------------------------------------------------------
  void Ring::OnPartnerKilled (Partner *partener)
  {
    Remove (partener);
  }

  // --------------------------------------------------------------------------------
  const gchar *Ring::GetRoleImage ()
  {
    if (_role == RESOURCE_MANAGER)
    {
      return "RESOURCE_MANAGER";
    }
    else if (_role == RESOURCE_USER)
    {
      return "RESOURCE_USER";
    }

    return NULL;
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

  // --------------------------------------------------------------------------------
  gboolean Ring::SendHeartbeat (Ring *ring)
  {
    Message *heartbeat = new Message ("Heartbeat");

    heartbeat->Set ("partner", ring->_partner_id);

    for (GList *current = ring->_partner_list; current; current = g_list_next (current))
    {
      Partner *partner = (Partner *) current->data;

      partner->SendMessage (heartbeat);
    }

    return G_SOURCE_CONTINUE;
  }
}
