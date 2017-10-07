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

class Partner;

namespace Net
{
  class Message;

  class Ring : public Object,
               public Object::Listener
  {
    public:
      class Listener
      {
        public:
          virtual void OnPartnerJoined (Partner  *partner,
                                        gboolean  joined) = 0;
      };

    public:
      static Ring *_broker;

      static void Join (const gchar *role,
                        guint        unicast_port,
                        GtkWidget   *partner_indicator);

      void Leave ();

      void Handshake (Message *message);

      void SpreadMessage (Message *message);

      void RecallMessage (Message *message);

      const gchar *GetRole ();

      void RegisterListener (Listener *listener);

      void UnregisterListener (Listener *listener);

      void NotifyPartnerStatus (Partner  *partner,
                                gboolean  join);

      const gchar *GetIpV4Address ();

    private:
      Ring (const gchar *role,
            guint        unicast_port,
            GtkWidget   *partner_indicator);

      virtual ~Ring ();

    private:
      static const gchar *ANNOUNCE_GROUP;
      static const guint  ANNOUNCE_PORT = 35000;

      gchar          *_role;
      gchar          *_ip_address;
      guint           _unicast_port;
      GList          *_partner_list;
      GList          *_message_list;
      GtkWidget      *_partner_indicator;
      GList          *_listeners;
      GSocketAddress *_multicast_address;

      void Add (Partner *partner);

      void Remove (const gchar *role);

      void Synchronize (Partner *partner);

      void AnnounceAvailability ();

      void Multicast (Message *message);

      gboolean JoinMulticast (GSocket *socket);

      static gboolean OnMulticast (GSocket      *socket,
                                   GIOCondition  condition,
                                   Ring         *ring);

      void Send (Message *message);

      gchar *GuessIpV4Address ();

      void OnObjectDeleted (Object *object);
  };
}
