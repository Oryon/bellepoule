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

#include "util/object.hpp"
#include "util/global.hpp"
#include "util/user_config.hpp"
#include "util/wifi_code.hpp"
#include "webapp/webapp_server.hpp"
#include "console/console_server.hpp"
#include "credentials.hpp"
#include "message.hpp"
#include "cryptor.hpp"
#include "usb_broker.hpp"
#include "usb_drive.hpp"
#include "usb_challenge.hpp"
#include "ring.hpp"

#ifndef G_OS_WIN32
#include <ifaddrs.h>
#endif

//#define MULTICAST_ANNOUNCE
#define WITH_BACKUP_CREDENTIALS

namespace Net
{
  const gchar *Ring::SECRET = "tagada soinsoin";
  Ring        *Ring::_broker = nullptr;

  // --------------------------------------------------------------------------------
  Ring::Ring (Role       role,
              Listener  *listener,
              GtkWidget *partner_indicator)
    : Object ("Ring")
  {
    _unicast_address   = nullptr;
    _partner_list      = nullptr;
    _partner_messages  = nullptr;
    _partner_listeners = nullptr;
    _web_app_messages  = nullptr;
    _listener          = listener;
    _announce_address  = nullptr;
    _role              = role;
    _unicast_port      = WifiCode::ClaimIpPort ();
    _credentials       = nullptr;
    _partner_id        = g_random_int ();
    _quit_countdown    = -1;
    _handshake_result  = HandshakeResult::NOT_APPLICABLE;
    _trusted_drive     = nullptr;

    _partner_indicator = partner_indicator;

    _usb_broker = new Net::UsbBroker (this);

    {
      gchar *passphrase = g_key_file_get_string (Global::_user_config->_key_file,
                                                 "Ring", "passphrase",
                                                 nullptr);

      if (passphrase == nullptr)
      {
        passphrase = FlashCode::GetKey256 ();
      }

      ChangePassphrase (passphrase);
      g_free (passphrase);
    }

#ifdef DEBUG
    if (_role == Role::RESOURCE_MANAGER)
    {
      _console_server = new Net::ConsoleServer (1234);
    }
    else
    {
      _console_server = new Net::ConsoleServer (4321);
    }
#endif

    Net::ConsoleServer::ExposeVariable ("heartbeat", 1);

    // Unicast listeners
    _http_server = new Net::HttpServer (this,
                                        _unicast_port);
    _web_app_server = new Net::WebAppServer (this,
                                             (guint) Channel::WEB_SOCKET,
                                             8000);

    // Announce listener
    {
      GError  *error  = nullptr;
      GSocket *socket;

      socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
                             G_SOCKET_TYPE_DATAGRAM,
                             G_SOCKET_PROTOCOL_DEFAULT,
                             nullptr);

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
                                                  nullptr);

        g_source_set_callback (source,
                               (GSourceFunc) OnMulticast,
                               this,
                               nullptr);
        g_source_attach (source, nullptr);
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
    _usb_broker->Release ();
    _credentials->Release ();
    _http_server->Release ();
    _web_app_server->Release ();

    TryToRelease (_console_server);

    TryToRelease (_trusted_drive);

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
      message->Set ("role",    (guint) _role);

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

    message->Set ("role",         (guint) _role);
    message->Set ("ip_address",   _unicast_address);
    message->Set ("unicast_port", _unicast_port);
    message->Set ("partner",      _partner_id);

    // Add secret challenge to message
    {
      Cryptor *cryptor = new Cryptor ();

      if (   (_handshake_result == HandshakeResult::NOT_APPLICABLE)
          || (_handshake_result == HandshakeResult::BACKUP_CHALLENGE_PASSED)
          || (_handshake_result == HandshakeResult::USB_TRUSTED))
      {
        Credentials *credentials = RetreiveBackupCredentials ();

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

        if (credentials)
        {
          gchar *iv;
          gchar *challenge = cryptor->Encrypt (SECRET,
                                               credentials->GetKey (),
                                               &iv);

          message->Set ("backup_challenge", challenge);
          message->Set ("backup_iv",        iv);

          g_free (iv);
          g_free (challenge);
          credentials->Release ();
        }
      }
      else if (_handshake_result == HandshakeResult::AUTHENTICATION_FAILED)
      {
        if (_trusted_drive)
        {
          gchar *iv;
          gchar *challenge = cryptor->Encrypt (SECRET,
                                               _trusted_drive->GetSerialNumber (),
                                               &iv);

          message->Set ("usb_challenge", challenge);
          message->Set ("usb_iv",        iv);

          g_free (iv);
          g_free (challenge);
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
    GError *error = nullptr;

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
                        nullptr,
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
      GError             *error       = nullptr;
      static const guint  buffer_size = 2048;
      gchar              *buffer      = g_new0 (gchar, buffer_size-1);

      g_socket_receive (socket,
                        buffer,
                        buffer_size,
                        nullptr,
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
                if (ring->DecryptSecret (message,
                                         nullptr,
                                         ring->_credentials))
                {
                  ring->Add (partner);
                  ring->SendHandshake (partner,
                                       HandshakeResult::CHALLENGE_PASSED);
                }
                else
                {
#ifdef WITH_BACKUP_CREDENTIALS
                  if (ring->DecryptSecret (message,
                                           "backup",
                                           ring->_credentials))
                  {
                    ring->SendHandshake (partner,
                                         HandshakeResult::BACKUP_CHALLENGE_PASSED);
                  }
                  else
#endif
                  if (message->HasField ("usb_challenge"))
                  {
                    UsbChallenge::Monitor (partner,
                                           message->GetString ("usb_challenge"),
                                           message->GetString ("usb_iv"));
                  }
                  else
                  {
                    ring->SendHandshake (partner,
                                         HandshakeResult::AUTHENTICATION_FAILED);
                  }
                }
              }
            }
            else
            {
              ring->SendHandshake (partner,
                                   HandshakeResult::ROLE_REJECTED);
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
    Credentials *credentials = nullptr;
    gchar       *partner_pass;

    {
      UserConfig *partner_config;

      if (_role == Role::RESOURCE_USER)
      {
        partner_config = new UserConfig ("BellePoule2D", TRUE);
      }
      else
      {
        partner_config = new UserConfig ("BellePoule", TRUE);
      }

      partner_pass = g_key_file_get_string (partner_config->_key_file,
                                            "Ring", "passphrase",
                                            nullptr);
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

    if ((result == HandshakeResult::CHALLENGE_PASSED) || (result == HandshakeResult::BACKUP_CHALLENGE_PASSED))
    {
      message->Set ("role",         (guint) _role);
      message->Set ("ip_address",   _unicast_address);
      message->Set ("unicast_port", _unicast_port);
      message->Set ("partner",      _partner_id);
    }

    message->Set ("response", (guint) result);

    message->SetFitness (1);
    partner->SendMessage (message);
    message->Release ();
  }

  // -------------------------------------------------------------------------------
  gboolean Ring::RoleIsAcceptable (Role partner_role)
  {
    if (partner_role == Role::RESOURCE_MANAGER)
    {
      if (_role == Role::RESOURCE_MANAGER)
      {
        return FALSE;
      }

      for (GList *current = _partner_list; current; current = g_list_next (current))
      {
        Partner *p = (Partner *) current->data;

        if (p->HasRole ((guint) Role::RESOURCE_MANAGER))
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
    GList *current = g_list_last (_partner_messages);

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
    Message *message = (Message *) object;

    _partner_messages = g_list_remove (_partner_messages,
                                       message);
    _web_app_messages = g_list_remove (_web_app_messages,
                                       message);
  }

  // -------------------------------------------------------------------------------
  void Ring::InjectMessage (Message *message,
                            Message *after)
  {
    GList *node = g_list_find (_partner_messages,
                               after);

    if (node)
    {
      message->SetFitness (1);
      message->AddObjectListener (this);

      if (node)
      {
        _partner_messages = g_list_insert_before (_partner_messages,
                                                  node,
                                                  message);
      }
    }
  }

  // -------------------------------------------------------------------------------
  void Ring::SpreadMessage (Message *message)
  {
    Channel channel = Channel::HTTP;

    if (message->HasField ("channel"))
    {
      channel = (Channel) message->GetInteger ("channel");
    }

    message->SetFitness (1);

    if (channel == Channel::WEB_SOCKET)
    {
      if (message->GetInteger ("client") == 0)
      {
        if (g_list_find (_web_app_messages,
                         message) == nullptr)
        {
          message->AddObjectListener (this);
          _web_app_messages = g_list_append (_web_app_messages,
                                             message);
        }
      }
      _web_app_server->SendMessageTo (message->GetInteger ("client"),
                                      message);
    }
    else
    {
      if (g_list_find (_partner_messages,
                       message) == nullptr)
      {
        message->AddObjectListener (this);
        _partner_messages = g_list_prepend (_partner_messages,
                                            message);
      }
      Send (message);
    }
  }

  // -------------------------------------------------------------------------------
  void Ring::RecallMessage (Message *message)
  {
    Channel channel = Channel::HTTP;

    if (message->HasField ("channel"))
    {
      channel = (Channel) message->GetInteger ("channel");
    }

    if (channel == Channel::WEB_SOCKET)
    {
      _web_app_messages = g_list_remove (_web_app_messages,
                                         message);

      if (message->GetFitness ())
      {
        message->SetFitness (0);
        _web_app_server->SendMessageTo (message->GetInteger ("client"),
                                        message);
      }
    }
    else
    {
      _partner_messages = g_list_remove (_partner_messages,
                                         message);

      if (message->GetFitness ())
      {
        message->SetFitness (0);
        Send (message);
      }
    }
  }

  // -------------------------------------------------------------------------------
  void Ring::RegisterPartnerListener (PartnerListener *listener)
  {
    if (g_list_find (_partner_listeners,
                     listener) == nullptr)
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
    return nullptr;
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
  gboolean Ring::DecryptSecret (Message     *message,
                                const gchar *type,
                                Credentials *credentials)
  {
    gboolean  result          = FALSE;
    GString  *challenge_field = g_string_new ("challenge");
    GString  *iv_field        = g_string_new ("iv");

    if (type)
    {
      g_string_prepend_c (challenge_field, '_');
      g_string_prepend   (challenge_field, type);
      g_string_prepend_c (iv_field, '_');
      g_string_prepend   (iv_field, type);
    }

    {
      gchar *challenge = message->GetString (challenge_field->str);
      gchar *iv        = message->GetString (iv_field->str);

      if (challenge && iv)
      {
        result = DecryptSecret (challenge,
                                iv,
                                credentials);
      }

      g_free (challenge);
      g_free (iv);
    }

    g_string_free (challenge_field, TRUE);
    g_string_free (iv_field,        TRUE);

    return result;
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
  void Ring::OnUsbEvent (UsbBroker::Event  event,
                         UsbDrive         *drive)
  {
    if (event == UsbBroker::Event::STICK_PLUGGED)
    {
      UsbChallenge::CheckKey (drive->GetSerialNumber (),
                              SECRET,
                              this);
    }
    else if (event == UsbBroker::Event::STICK_UNPLUGGED)
    {
      if (_handshake_result == HandshakeResult::AUTHENTICATION_FAILED)
      {
        TryToRelease (_trusted_drive);
        _trusted_drive = drive;
        _trusted_drive->Retain ();

        AnnounceAvailability ();
      }
    }

    _listener->OnUsbEvent (event,
                           drive);
  }

  // --------------------------------------------------------------------------------
  void Ring::OnUsbPartnerTrusted (Partner     *partner,
                                  const gchar *key)
  {
    Message *message = new Message ("Ring::Handshake");

    message->Set ("response", (guint) HandshakeResult::USB_TRUSTED);
    message->Set ("password", GetCryptorKey ());

    message->SetFitness (1);
    message->SetPassPhrase256 (key);

    partner->SendMessage (message);
    message->Release ();
  }

  // --------------------------------------------------------------------------------
  void Ring::OnUsbPartnerDoubtful (Partner *partner)
  {
    printf ("==>> Doubt !!!!\n");
  }

  // --------------------------------------------------------------------------------
  const gchar *Ring::GetRoleImage ()
  {
    if (_role == Role::RESOURCE_MANAGER)
    {
      return "RESOURCE_MANAGER";
    }
    else if (_role == Role::RESOURCE_USER)
    {
      return "RESOURCE_USER";
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  const gchar *Ring::GetSecretKey (const gchar *authentication_scheme)
  {
    if (authentication_scheme)
    {
      if (   (g_strcmp0 (authentication_scheme, "/ring") == 0)
          || (g_strcmp0 (authentication_scheme, "/sender/0") == 0))
      {
        if (_trusted_drive)
        {
          return _trusted_drive->GetSerialNumber ();
        }
        else
        {
          return GetCryptorKey ();
        }
      }
      else
      {
        return _listener->GetSecretKey (authentication_scheme);
      }
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  gboolean Ring::OnMessage (Net::Message *message)
  {
    if (message->Is ("Ring::Handshake"))
    {
      _handshake_result = (HandshakeResult) message->GetInteger ("response");

      if (_handshake_result == HandshakeResult::BACKUP_CHALLENGE_PASSED)
      {
        Credentials *credentials = RetreiveBackupCredentials ();

        ChangePassphrase (credentials->GetKey ());
        AnnounceAvailability ();
        credentials->Release ();
      }
      else
      {
        if (_handshake_result == HandshakeResult::CHALLENGE_PASSED)
        {
          Partner *partner = new Partner (message,
                                          this);

          Add (partner);

          partner->Release ();
        }
        else if (_handshake_result == HandshakeResult::USB_TRUSTED)
        {
          gchar *password = message->GetString ("password");

          ChangePassphrase (password);
          g_free (password);

          TryToRelease (_trusted_drive);
          _trusted_drive = nullptr;

          AnnounceAvailability ();
        }

        _listener->OnHanshakeResult (_handshake_result);
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
    else if (message->Is ("SmartPoule::JobListCall"))
    {
      guint client = message->GetInteger ("client");

      for (GList *m = _web_app_messages; m; m = g_list_next (m))
      {
        Message *webapp_message = (Message *) m->data;

        _web_app_server->SendMessageTo (client,
                                        webapp_message);
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
    const gchar *broadcast_address = nullptr;

#ifdef G_OS_WIN32
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
            if ((pAdapter->Type == IF_TYPE_IEEE80211) || (pAdapter->Type == MIB_IF_TYPE_ETHERNET))
            {
              if (g_strcmp0 (pAdapter->IpAddressList.IpAddress.String, "0.0.0.0") != 0)
              {
                g_free (_unicast_address);
                _unicast_address = g_strdup (pAdapter->IpAddressList.IpAddress.String);

                {
                  struct in_addr iaddr;
                  gulong         ip   = inet_addr (_unicast_address);
                  gulong         mask = inet_addr (pAdapter->IpAddressList.IpMask.String);

                  iaddr.S_un.S_addr = ip | ~mask;

                  broadcast_address = inet_ntoa (iaddr);
                }

                if (pAdapter->Type == IF_TYPE_IEEE80211)
                {
                  break;
                }
              }
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
      for (struct ifaddrs *ifa = ifa_list; ifa != nullptr; ifa = ifa->ifa_next)
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
                             nullptr, 0,
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

    if (_unicast_address == nullptr)
    {
      GInetAddress *loopback = g_inet_address_new_loopback (G_SOCKET_FAMILY_IPV4);

      _unicast_address  = g_inet_address_to_string (loopback);
      broadcast_address = _unicast_address;
      g_object_unref (loopback);
    }

    if (_credentials)
    {
      _credentials->SetIpAddress (_unicast_address);
    }

    if (broadcast_address)
    {
      GInetAddress *inet_address = g_inet_address_new_from_string (broadcast_address);

      _announce_address = g_inet_socket_address_new (inet_address,
                                                     ANNOUNCE_PORT);
      g_object_unref (inet_address);
    }

    _web_app_server->SetIpV4Address (_unicast_address);
  }

  // --------------------------------------------------------------------------------
  gboolean Ring::SendHeartbeat (Ring *ring)
  {
    if (Net::ConsoleServer::VariableIsSet ("heartbeat"))
    {
      Message *heartbeat = new Message ("Ring::Heartbeat");

      heartbeat->Set ("partner", ring->_partner_id);

      for (GList *current = ring->_partner_list; current; current = g_list_next (current))
      {
        Partner *partner = (Partner *) current->data;

        partner->SendMessage (heartbeat);
      }

      heartbeat->Release ();
    }

    return G_SOURCE_CONTINUE;
  }
}
