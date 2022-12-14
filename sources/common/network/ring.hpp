// Copyright (C) 2011 Yannick Le Roux.
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

#pragma once

#include <gtk/gtk.h>

#include <util/object.hpp>
#include <http_server.hpp>
#include <server.hpp>
#include <partner.hpp>
#include <usb_broker.hpp>
#include <usb_challenge.hpp>

namespace Net
{
  class Message;
  class Credentials;
  class ConsoleServer;
  class UsbDrive;
  class WebAppServer;

  class Ring : public Object,
               public Object::Listener,
               public Server::Listener,
               public Partner::Listener,
               public UsbBroker::Listener,
               public UsbChallenge::Listener
  {
    public:
      enum class HandshakeResult
      {
        NOT_APPLICABLE,
        CHALLENGE_PASSED,
        BACKUP_CHALLENGE_PASSED,
        AUTHENTICATION_FAILED,
        USB_TRUSTED,
        ROLE_REJECTED
      };

      enum class Role
      {
        RESOURCE_MANAGER,
        RESOURCE_USER
      };

      enum class Channel
      {
        WEB_SOCKET,
        HTTP
      };

      struct PartnerListener
      {
        virtual void OnPartnerJoined (Partner  *partner,
                                      gboolean  joined) = 0;
      };

      struct Listener
      {
        virtual gboolean     OnMessage        (Message *message)                   = 0;
        virtual const gchar *GetSecretKey     (const gchar *authentication_scheme) = 0;
        virtual void         OnHanshakeResult (HandshakeResult result)             = 0;
        virtual void         OnUsbEvent       (UsbBroker::Event  event,
                                               UsbDrive         *drive)            = 0;
      };

    public:
      static Ring *_broker;

      static void Join (Role       role,
                        Listener  *listener,
                        GtkWidget *partner_indicator);

      void Leave ();

      void InjectMessage (Message *message,
                          Message *after);

      void SpreadMessage (Message *message);

      void RecallMessage (Message *message);

      const gchar *GetCryptorKey ();

      void RegisterPartnerListener (PartnerListener *listener);

      void UnregisterPartnerListener (PartnerListener *listener);

      void NotifyPartnerStatus (Partner  *partner,
                                gboolean  join);

      Partner *GetPartner ();

      FlashCode *GetFlashCode ();

      const gchar *GetIpV4Address ();

      void AnnounceAvailability ();

      void ChangePassphrase (const gchar *passphrase);

      Credentials *RetreiveBackupCredentials ();

      void StampSender (Message *message);

    private:
      Ring (Role       role,
            Listener  *listener,
            GtkWidget  *partner_indicator);

      ~Ring () override;

    private:
      static const guint  ANNOUNCE_PORT = 35830;
      static const gchar *SECRET;

      Role             _role;
      guint            _heartbeat_timer;
      guint32          _partner_id;
      gchar           *_unicast_address;
      guint            _unicast_port;
      GList           *_partner_list;
      GList           *_partner_messages;
      GtkWidget       *_partner_indicator;
      GList           *_partner_listeners;
      GList           *_web_app_messages;
      Listener        *_listener;
      GSocketAddress  *_announce_address;
      GSocket         *_announce_socket;
      Credentials     *_credentials;
      HttpServer      *_http_server;
      WebAppServer    *_web_app_server;
      gint             _quit_countdown;
      UsbBroker       *_usb_broker;
      HandshakeResult  _handshake_result;
      UsbDrive        *_trusted_drive;
      ConsoleServer   *_console_server    = nullptr;

      void Add (Partner *partner);

      gboolean RoleIsAcceptable (Role partner_role);

      const gchar *GetSecretKey (const gchar *authentication_scheme) override;

      gboolean OnMessage (Net::Message *message) override;

      void Remove (Partner *partner);

      void Synchronize (Partner *partner);

      void Multicast (Message *message);

      gboolean JoinMulticast (GSocket *socket);

      static gboolean OnMulticast (GSocket      *socket,
                                   GIOCondition  condition,
                                   Ring         *ring);

      void Send (Message *message);

      void GuessIpV4Addresses ();

      void OnObjectDeleted (Object *object) override;

      void DisplayIndicator ();

      void SendHandshake (Partner         *partner,
                          HandshakeResult  result);

      gboolean DecryptSecret (Message     *message,
                              const gchar *type,
                              Credentials *credentials);

      gboolean DecryptSecret (gchar       *crypted,
                              gchar       *iv,
                              Credentials *credentials);

      const gchar *GetRoleImage ();

      void OnPartnerKilled (Partner *partener) override;

      void OnPartnerLeaved (Partner *partener) override;

      void OnUsbEvent (UsbBroker::Event  event,
                       UsbDrive         *drive) override;

      void OnUsbPartnerTrusted (Partner     *partner,
                                const gchar *key) override;

      void OnUsbPartnerDoubtful (Partner *partner) override;

      static gboolean SendHeartbeat (Ring *ring);
  };
}
