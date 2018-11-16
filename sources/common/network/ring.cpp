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
#include "util/wifi_code.hpp"
#include "credentials.hpp"
#include "message.hpp"
#include "cryptor.hpp"
#include "ring.hpp"

//#define MULTICAST_ANNOUNCE
#define WITH_BACKUP_CREDENTIALS

namespace Net
{
  const gchar *Ring::SECRET = "tagada soinsoin";

  Ring *Ring::_broker = NULL;

  // --------------------------------------------------------------------------------
  Ring::Ring (Role       role,
              Listener  *listener,
              GtkWidget *partner_indicator)
    : Object ("Ring")
  {
    _unicast_address   = NULL;
    _partner_list      = NULL;
    _message_list      = NULL;
    _partner_listeners = NULL;
    _listener          = listener;
    _announce_address  = NULL;
    _role              = role;
    _unicast_port      = WifiCode::ClaimIpPort ();
    _credentials       = NULL;
    _partner_id        = g_random_int ();
    _quit_countdown    = -1;

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

    // Unicast listener
    _http_server = new Net::HttpServer (this,
                                        _unicast_port);

    // Announce listener
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
    _credentials->Release ();
    _http_server->Release ();

    g_free (_unicast_address);
    g_object_unref (_announce_address);

    gtk_main_quit ();
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
  const gchar *Ring::GetCryptorKey ()
  {
    return _credentials->GetKey ();
  }

  // --------------------------------------------------------------------------------
  void Ring::Join (Role       role,
                   Listener  *listener,
                   GtkWidget  *partner_indicator)
  {
    if (_broker)
    {
      g_warning ("Ring instanciated more than once");
    }

    _broker = new Ring (role,
                        listener,
                        partner_indicator);

    _broker->AnnounceAvailability ();
  }

  // --------------------------------------------------------------------------------
  gboolean Ring::JoinMulticast (GSocket *socket)
  {
    GuessIpV4Addresses ();

#ifdef MULTICAST_ANNOUNCE
    if (_announce_address)
    {
      g_object_unref (_announce_address);
    }

    {
      GError       *error   = NULL;
      GInetAddress *address = g_inet_address_new_from_string ("225.0.0.35");

      g_socket_join_multicast_group (socket,
                                     address,
                                     FALSE,
                                     NULL,
                                     &error);
      if (error)
      {
        g_warning ("Ring::JoinMulticast: %s", error->message);
        g_clear_error (&error);

        g_object_unref (address);
        address = g_inet_address_new_loopback (G_SOCKET_FAMILY_IPV4);
      }

      _announce_address = g_inet_socket_address_new (address,
                                                     ANNOUNCE_PORT);
      g_object_unref (address);
    }
#endif

    return TRUE;
  }

  // --------------------------------------------------------------------------------
  void Ring::Leave ()
  {
    g_object_unref (_announce_socket);

    if (_heartbeat_timer)
    {
      g_source_remove (_heartbeat_timer);
      _heartbeat_timer = 0;
    }

    {
      Message *message = new Message ("Ring::Farewell");

      message->Set ("partner", _partner_id);
      message->Set ("role",    _role);

      _quit_countdown = 0;
      for (GList *current = _partner_list; current; current = _partner_list)
      {
        Partner *partner = (Partner *) current->data;

        partner->SendMessage (message);
        Remove (partner);
        _quit_countdown++;
      }

      message->Release ();
    }

    if (_quit_countdown == 0)
    {
      Release ();
    }
  }

  // --------------------------------------------------------------------------------
  void Ring::AnnounceAvailability ()
  {
    Message *message = new Message ("Ring::Announcement");

    message->Set ("role",         _role);
    message->Set ("ip_address",   _unicast_address);
    message->Set ("unicast_port", _unicast_port);
    message->Set ("partner",      _partner_id);

    // Add secret challenge to message
    {
      Cryptor *cryptor = new Cryptor ();

      {
        gchar *iv;
        gchar *challenge = cryptor->Encrypt (SECRET,
                                             _credentials->GetKey (),
                                             &iv);
        message->Set ("challenge", challenge);
        message->Set ("iv", iv);

        g_free (iv);
        g_free (challenge);
      }

      {
        Credentials *credentials = RetreiveBackupCredentials ();

        if (credentials)
        {
          gchar       *backup_iv;
          gchar       *backup_challenge  = cryptor->Encrypt (SECRET,
                                                             credentials->GetKey (),
                                                             &backup_iv);

          message->Set ("backup_challenge", backup_challenge);
          message->Set ("backup_iv",        backup_iv);

          g_free (backup_iv);
          g_free (backup_challenge);
          credentials->Release ();
        }
      }

      cryptor->Release ();
    }

    Multicast (message);
    message->Release ();
  }

  // --------------------------------------------------------------------------------
  void Ring::Multicast (Message *message)
  {
    GError *error = NULL;

    _announce_socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
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

      g_socket_set_blocking  (_announce_socket, FALSE);
      g_socket_set_broadcast (_announce_socket, TRUE);

      g_socket_send_to (_announce_socket,
                        _announce_address,
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
      GError             *error       = NULL;
      static const guint  buffer_size = 2048;
      gchar              *buffer      = g_new0 (gchar, buffer_size-1);

      g_socket_receive (socket,
                        buffer,
                        buffer_size,
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
        guint32  id      = message->GetInteger ("partner");

        if (message->Is ("Ring::Announcement"))
        {
          if (ring->_partner_id != id)
          {
            Role     role    = (Role) message->GetInteger ("role");
            Partner *partner = new Partner (message, ring);

            if (ring->RoleIsAcceptable (role))
            {
              if (role != ring->_role)
              {
                gchar *challenge = message->GetString  ("challenge");
                gchar *iv         = message->GetString ("iv");

                if (ring->DecryptSecret (challenge,
                                         iv,
                                         ring->_credentials))
                {
                  ring->Add (partner);
                  ring->SendHandshake (partner,
                                       CHALLENGE_PASSED);
                }
                else
                {
#ifdef WITH_BACKUP_CREDENTIALS
                  gchar *backup_challenge = message->GetString ("backup_challenge");
                  gchar *backup_iv        = message->GetString ("backup_iv");

                  if (ring->DecryptSecret (backup_challenge,
                                           backup_iv,
                                           ring->_credentials))
                  {
                    ring->SendHandshake (partner,
                                         BACKUP_CHALLENGE_PASSED);
                  }
                  else
                  {
                    ring->SendHandshake (partner,
                                         AUTHENTICATION_FAILED);
                  }

                  g_free (backup_challenge);
                  g_free (backup_iv);
#else
                  ring->SendHandshake (partner,
                                       AUTHENTICATION_FAILED);
#endif
                }
                g_free (challenge);
                g_free (iv);
              }
            }
            else
            {
              ring->SendHandshake (partner,
                                   ROLE_REJECTED);
            }

            partner->Release ();
          }
        }
        else if (message->Is ("SmartPoule::ScoreSheetCall"))
        {
          gchar   *hidden_text;
          Message *hidden_message;

          {
            Cryptor     *cryptor = new Cryptor ();
            gchar       *b64_iv  = message->GetString ("IV");
            gchar       *cypher  = message->GetString ("cypher");
            gchar       *sender  = message->GetString ("sender");
            const gchar *key     = ring->_listener->GetSecretKey (sender);

            hidden_text = cryptor->Decrypt (cypher,
                                            b64_iv,
                                            key);

            g_free (cypher);
            g_free (sender);
            g_free (b64_iv);
            cryptor->Release ();
          }

          hidden_message = new Message ((const guint8*) hidden_text);
          ring->_listener->OnMessage (hidden_message);

          hidden_message->Release ();
          g_free (hidden_text);
        }

        message->Release ();
      }
      g_free (buffer);
    }

    return G_SOURCE_CONTINUE;
  }

  // -------------------------------------------------------------------------------
  Credentials *Ring::RetreiveBackupCredentials ()
  {
    Credentials *credentials = NULL;
    gchar       *partner_pass;

    {
      UserConfig *partner_config;

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
      partner_config->Release ();
    }

    if (partner_pass)
    {
      credentials = new Credentials (GetRoleImage (),
                                      _unicast_address,
                                      _unicast_port,
                                      partner_pass);

      g_free (partner_pass);
    }

    return credentials;
  }

  // -------------------------------------------------------------------------------
  void Ring::StampSender (Message *message)
  {
    gchar *url = g_strdup_printf ("http://%s:%d", _unicast_address, _unicast_port);

    message->Set ("address", url);
    g_free (url);
  }

  // -------------------------------------------------------------------------------
  void Ring::SendHandshake (Partner         *partner,
                            HandshakeResult  result)
  {
    Message *message = new Message ("Ring::Handshake");

    if ((result == CHALLENGE_PASSED) || (result == BACKUP_CHALLENGE_PASSED))
    {
      message->Set ("role",         _role);
      message->Set ("ip_address",   _unicast_address);
      message->Set ("unicast_port", _unicast_port);
      message->Set ("partner",      _partner_id);
    }

    message->Set ("response", result);

    message->SetFitness (1);
    partner->SendMessage (message);
    message->Release ();
  }

  // -------------------------------------------------------------------------------
  gboolean Ring::RoleIsAcceptable (Role partner_role)
  {
    if (partner_role == RESOURCE_MANAGER)
    {
      if (_role == RESOURCE_MANAGER)
      {
        return FALSE;
      }

      for (GList *current = _partner_list; current; current = g_list_next (current))
      {
        Partner *p = (Partner *) current->data;

        if (p->HasRole (RESOURCE_MANAGER))
        {
          return FALSE;
        }
      }
    }
    return TRUE;
  }

  // -------------------------------------------------------------------------------
  void Ring::Add (Partner *partner)
  {
    partner->SetPassPhrase256 (_credentials->GetKey ());

    partner->Retain ();
    _partner_list = g_list_prepend (_partner_list,
                                    partner);
    NotifyPartnerStatus (partner,
                         TRUE);
    Synchronize (partner);

    DisplayIndicator ();
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
      partner->Leave ();
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

    _message_list = g_list_remove (_message_list,
                                   message);
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
    _message_list = g_list_remove (_message_list,
                                   message);

    if (message->GetFitness ())
    {
      message->SetFitness (0);
      Send (message);
    }
  }

  // -------------------------------------------------------------------------------
  void Ring::RegisterPartnerListener (PartnerListener *listener)
  {
    if (g_list_find (_partner_listeners,
                     listener) == NULL)
    {
      _partner_listeners = g_list_prepend (_partner_listeners,
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
  void Ring::UnregisterPartnerListener (PartnerListener *listener)
  {
    _partner_listeners = g_list_remove (_partner_listeners,
                                        listener);
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
    GList *current = _partner_listeners;

    while (current)
    {
      PartnerListener *listener = (PartnerListener *) current->data;

      listener->OnPartnerJoined (partner,
                                 joined);

      current = g_list_next (current);
    }
  }

  // --------------------------------------------------------------------------------
  void Ring::ChangePassphrase (const gchar *passphrase)
  {
    g_key_file_set_string (Global::_user_config->_key_file,
                           "Ring", "passphrase", passphrase);

    Object::TryToRelease (_credentials);
    _credentials = new Credentials (GetRoleImage (),
                                    _unicast_address,
                                    _unicast_port,
                                    passphrase);

    Global::_user_config->Save ();
  }

  // --------------------------------------------------------------------------------
  FlashCode *Ring::GetFlashCode ()
  {
    return _credentials;
  }

  // --------------------------------------------------------------------------------
  gboolean Ring::DecryptSecret (gchar       *crypted,
                                gchar       *iv,
                                Credentials *credentials)
  {
    gboolean decoded = FALSE;
    Cryptor *cryptor = new Cryptor ();
    gchar   *secret;

    secret = cryptor->Decrypt (crypted,
                               iv,
                               credentials->GetKey ());

    if (g_ascii_strcasecmp (secret, SECRET) == 0)
    {
      decoded = TRUE;
    }

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
  void Ring::OnPartnerLeaved (Partner *partener)
  {
    partener->Release ();

    if (_quit_countdown != -1)
    {
      _quit_countdown--;

      if (_quit_countdown == 0)
      {
        Release ();
      }
    }
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
  const gchar *Ring::GetSecretKey (const gchar *authentication_scheme)
  {
    if (authentication_scheme)
    {
      if (   (g_strcmp0 (authentication_scheme, "/ring") == 0)
          || (g_strcmp0 (authentication_scheme, "/sender/0") == 0))
      {
        return GetCryptorKey ();
      }
      else
      {
        return _listener->GetSecretKey (authentication_scheme);
      }
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  gboolean Ring::OnMessage (Net::Message *message)
  {
    if (message->Is ("Ring::Handshake"))
    {
      HandshakeResult response = (HandshakeResult) message->GetInteger ("response");

      if (response == BACKUP_CHALLENGE_PASSED)
      {
        Credentials *credentials = RetreiveBackupCredentials ();

        ChangePassphrase (credentials->GetKey ());
        AnnounceAvailability ();
        credentials->Release ();
      }
      else
      {
        if (response == CHALLENGE_PASSED)
        {
          Partner *partner = new Partner (message,
                                          this);

          Add (partner);

          partner->Release ();
        }

        _listener->OnHanshakeResult (response);
      }
      return TRUE;
    }
    else if (message->Is ("Ring::Farewell"))
    {
      guint32 id = message->GetInteger ("partner");

      if (_partner_id != id)
      {
        for (GList *current = _partner_list; current; current = g_list_next (current))
        {
          Partner *partner = (Partner *) current->data;

          if (partner->Is (message->GetInteger ("partner")))
          {
            Remove (partner);
            break;
          }
        }
      }
      return TRUE;
    }
    else
    {
      return _listener->OnMessage (message);
    }
  }

  // --------------------------------------------------------------------------------
  const gchar *Ring::GetIpV4Address ()
  {
    return _unicast_address;
  }

  // --------------------------------------------------------------------------------
  void Ring::GuessIpV4Addresses ()
  {
    const gchar *broadcast_address = NULL;

#ifdef WIN32
#if 0
    {
      struct hostent *hostinfo;
      gchar           hostname[50];

      if (gethostname (hostname, sizeof (hostname)) == 0)
      {
        hostinfo = gethostbyname (hostname);
        if (hostinfo)
        {
          _unicast_address = g_strdup (inet_ntoa (*(struct in_addr *) (hostinfo->h_addr)));

          for (guint i = 0; hostinfo->h_addr_list[i] != 0; i++)
          {
            printf("\tIPv4 Address #%d: %s\n", i, inet_ntoa (*(struct in_addr *) (hostinfo->h_addr_list[i])));
          }
        }
      }
    }
#else
    {
      gulong info_length;

      if (GetAdaptersInfo (NULL, &info_length) == ERROR_BUFFER_OVERFLOW)
      {
        PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO *) g_malloc (info_length);

        if (GetAdaptersInfo(pAdapterInfo, &info_length) == NO_ERROR)
        {
          for (PIP_ADAPTER_INFO pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next)
          {
            if (g_strcmp0 (pAdapter->IpAddressList.IpAddress.String, "0.0.0.0") != 0)
            {
              _unicast_address = g_strdup (pAdapter->IpAddressList.IpAddress.String);

              {
                struct in_addr iaddr;
                gulong         ip   = inet_addr (_unicast_address);
                gulong         mask = inet_addr (pAdapter->IpAddressList.IpMask.String);

                iaddr.S_un.S_addr = ip | ~mask;

                broadcast_address = inet_ntoa (iaddr);
              }
              break;
            }
          }
        }

        g_free (pAdapterInfo);
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
            && (ifa->ifa_flags  & IFF_UP)
            && (ifa->ifa_flags  & IFF_RUNNING)
            && ((ifa->ifa_flags & IFF_LOOPBACK) == 0))
        {
          int family = ifa->ifa_addr->sa_family;

          if (family == AF_INET)
          {
            char host[NI_MAXHOST];

            if (getnameinfo (ifa->ifa_addr,
                             sizeof (struct sockaddr_in),
                             host, NI_MAXHOST,
                             NULL, 0,
                             NI_NUMERICHOST) == 0)
            {
              _unicast_address = g_strdup (host);
              broadcast_address = inet_ntoa (((struct sockaddr_in *) ifa->ifa_broadaddr)->sin_addr);
              break;
            }
          }
        }
      }

      freeifaddrs (ifa_list);
    }
#endif

    if (_unicast_address == NULL)
    {
      GInetAddress *loopback = g_inet_address_new_loopback (G_SOCKET_FAMILY_IPV4);

      _unicast_address  = g_inet_address_to_string (loopback);
      broadcast_address = _unicast_address;
      g_object_unref (loopback);
    }

    if (broadcast_address)
    {
      GInetAddress *inet_address = g_inet_address_new_from_string (broadcast_address);

      _announce_address = g_inet_socket_address_new (inet_address,
                                                     ANNOUNCE_PORT);
      g_object_unref (inet_address);
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Ring::SendHeartbeat (Ring *ring)
  {
    Message *heartbeat = new Message ("Ring::Heartbeat");

    heartbeat->Set ("partner", ring->_partner_id);

    for (GList *current = ring->_partner_list; current; current = g_list_next (current))
    {
      Partner *partner = (Partner *) current->data;

      partner->SendMessage (heartbeat);
    }

    heartbeat->Release ();

    return G_SOURCE_CONTINUE;
  }
}
