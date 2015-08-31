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

#ifndef crew_hpp
#define crew_hpp

#include <gtk/gtk.h>

class Partner;

namespace Net
{
  class Message;

  class Crew
  {
    public:
      static void Join (const gchar *role,
                        guint        unicast_port,
                        GtkWidget   *partner_indicator);

      static void Leave ();

      static void SendMessage (Message *message);

      static void DropMessage (Message *message);

      static void Handshake (Message *message);

    private:
      Crew ();
      virtual ~Crew ();

    private:
      static const gchar *ANNOUNCE_GROUP;
      static const guint  ANNOUNCE_PORT = 35000;

      static gchar     *_role;
      static guint      _unicast_port;
      static GList     *_partner_list;
      static GList     *_message_list;
      static GtkWidget *_partner_indicator;

      static gboolean OnMulticast (Message *message);

      static void Add (Partner *partner);

      static void Synchronize (Partner *partner);

      static void AnnounceAvailability ();

      static void Multicast (Message *message);

      static gpointer MulticastListener ();
  };
}
#endif
