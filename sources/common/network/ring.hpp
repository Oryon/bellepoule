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
#include <partner.hpp>

namespace Net
{
  class Message;
  class Credentials;

  class Ring : public Object,
               public Object::Listener,
               public HttpServer::Listener,
               public Partner::Listener
  {
    public:
      typedef enum
      {
        CHALLENGE_PASSED,
        BACKUP_CHALLENGE_PASSED,
        AUTHENTICATION_FAILED,
        ROLE_REJECTED
      } HandshakeResult;

      typedef enum
      {
        RESOURCE_MANAGER,
        RESOURCE_USER
      } Role;

      struct PartnerListener
      {
        virtual void OnPartnerJoined (Partner  *partner,
                                      gboolean  joined) = 0;
      };

      struct Listener
      {
        virtual gboolean     OnMessage        (Net::Message *message)              = 0;
        virtual const gchar *GetSecretKey     (const gchar *authentication_scheme) = 0;
        virtual void         OnHanshakeResult (HandshakeResult result)             = 0;
      };

    public:
      static Ring *_broker;

      static void Join (Role       role,
                        Listener  *listener,
                        GtkWidget *partner_indicator);

      void Leave ();

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

      virtual ~Ring ();

    private:
      static const guint  ANNOUNCE_PORT = 35830;
      static const gchar *SECRET;

      Role            _role;
      guint           _heartbeat_timer;
      gint32          _partner_id;
      gchar          *_unicast_address;
      guint           _unicast_port;
      GList          *_partner_list;
      GList          *_message_list;
      GtkWidget      *_partner_indicator;
      GList          *_partner_listeners;
      Listener       *_listener;
      GSocketAddress *_announce_address;
      Credentials    *_credentials;
      HttpServer     *_http_server;
      gint            _quit_countdown;

      void Add (Partner *partner);

      gboolean RoleIsAcceptable (Role partner_role);

      const gchar *GetSecretKey (const gchar *authentication_scheme);

      gboolean OnMessage (Net::Message *message);

      void Remove (Partner *partner);

      void Synchronize (Partner *partner);

      void Multicast (Message *message);

      gboolean JoinMulticast (GSocket *socket);

      static gboolean OnMulticast (GSocket      *socket,
                                   GIOCondition  condition,
                                   Ring         *ring);

      void Send (Message *message);

      void GuessIpV4Addresses ();

      void OnObjectDeleted (Object *object);

      void DisplayIndicator ();

      void SendHandshake (Partner         *partner,
                          HandshakeResult  result);

      gboolean DecryptSecret (gchar       *crypted,
                              gchar       *iv,
                              Credentials *credentials);

      const gchar *GetRoleImage ();

      void OnPartnerKilled (Partner *partener);

      void OnPartnerLeaved (Partner *partener);

      static gboolean SendHeartbeat (Ring *ring);
  };
}
