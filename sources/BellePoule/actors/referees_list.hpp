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

#ifndef referees_list_hpp
#define referees_list_hpp

#include <gtk/gtk.h>
#include <libxml/xpath.h>
#include <libxml/xmlwriter.h>

#include "actors/checkin.hpp"

class Weapon;

namespace People
{
  class RefereesList : public People::Checkin
  {
    public:
      RefereesList ();

      void SetWeapon (Weapon *weapon);

      const gchar *GetWeaponCode ();

      void ConvertFromBaseToResult ();

      void Expand ();

      void Collapse ();

    protected:
      virtual ~RefereesList ();

    private:
      guint32  _dnd_key;
      Filter  *_expanded_filter;
      Filter  *_collapsed_filter;
      Weapon  *_weapon;

      void Monitor (Player *referee);

      void OnPlayerLoaded (Player *referee,
                           Player *owner);

      void OnPlayerEventFromForm (Player                    *referee,
                                  People::Form::PlayerEvent  event);

      void on_players_list_row_activated (GtkTreePath *path);

      static gboolean RefereeIsVisible (GtkTreeModel *model,
                                        GtkTreeIter  *iter,
                                        RefereesList *referee_list);

      static void OnAvailabilityChanged (Player    *referee,
                                         Attribute *attr,
                                         Object    *owner,
                                         guint      step);

      static void OnConnectionChanged (Player    *referee,
                                       Attribute *attr,
                                       Object    *owner,
                                       guint      step);

      static void OnAttendingChanged (Player    *referee,
                                      Attribute *attr,
                                      Object    *owner,
                                      guint      step);

      static void OnParticipationRateChanged (Player    *referee,
                                              Attribute *attr,
                                              Object    *owner,
                                              guint      step);

      void OnDragDataGet (GtkWidget        *widget,
                          GdkDragContext   *drag_context,
                          GtkSelectionData *data,
                          guint             key,
                          guint             time);

      static void OnDisplayJobs (GtkWidget    *w,
                                 RefereesList *referee_list);
  };
}

#endif
