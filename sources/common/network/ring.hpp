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

#ifndef ring_hpp
#define ring_hpp

#include <gtk/gtk.h>

class Partner;

namespace Net
{
  class Message;

  class Ring
  {
    public:
      class Listener
      {
        public:
          virtual void OnMessage (Net::Message *message) = 0;
      };

    public:
      static void Join (const gchar *role,
                        guint        unicast_port,
                        GtkWidget   *partner_indicator);

      static void Leave ();

      static void Handshake (Message *message);

      static void SpreadMessage (Message *message);

      static void RecallMessage (Message *message);

      static const gchar *GetRole ();

      static void RegisterListener (Listener *listener);

      static void UnregisterListener (Listener *listener);

      static void PostToListener (Net::Message *message);

    private:
      Ring ();
      virtual ~Ring ();

    private:
      static const gchar *ANNOUNCE_GROUP;
      static const guint  ANNOUNCE_PORT = 35000;

      static gchar          *_role;
      static gchar          *_ip_address;
      static guint           _unicast_port;
      static GList          *_partner_list;
      static GList          *_message_list;
      static GtkWidget      *_partner_indicator;
      static GList          *_listeners;
      static GSocketAddress *_multicast_address;

      static void Add (Partner *partner);

      static void Remove (const gchar *role);

      static void Synchronize (Partner *partner);

      static void AnnounceAvailability ();

      static void Multicast (Message *message);

      static gboolean JoinMulticast (GSocket      *socket,
                                     GInetAddress *group);

      static gboolean OnMulticast (GSocket      *socket,
                                   GIOCondition  condition,
                                   gpointer      user_data);

      static void Send (Message *message);
  };
}
#endif
