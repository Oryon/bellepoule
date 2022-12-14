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

#pragma once

#include <gtk/gtk.h>
#include <libxml/xpath.h>

#include "actors/checkin.hpp"

class Weapon;
class Referee;

namespace People
{
  class RefereesList : public People::Checkin
  {
    public:
      struct Listener
      {
        virtual void OnOpenCheckin (RefereesList *referee_list) = 0;
        virtual void OnRefereeUpdated (RefereesList *referee_list,
                                       Player       *referee)   = 0;
      };

    public:
      RefereesList (Listener *listener = nullptr);

      void SetWeapon (Weapon *weapon);

      const gchar *GetWeaponCode ();

      void Expand ();

      void Collapse ();

      gboolean IsCollapsed ();

      GtkWidget *GetHeader ();

      void OnCheckinClicked ();

      void Add (Player *player) override;

    protected:
      ~RefereesList () override;

    private:
      Filter   *_expanded_filter;
      Filter   *_collapsed_filter;
      Weapon   *_weapon;
      Listener *_listener;

      void RefreshAttendingDisplay () override;

      void Monitor (Player *referee) override;

      void OnPlayerLoaded (Player *referee,
                           Player *owner) override;

      void OnFormEvent (Player              *referee,
                        People::Form::Event  event) override;

      void TogglePlayerAttr (Player              *player,
                             Player::AttributeId *attr_id,
                             gboolean             new_value,
                             gboolean             popup_on_error = FALSE) override;

      static gboolean RefereeIsVisible (GtkTreeModel *model,
                                        GtkTreeIter  *iter,
                                        RefereesList *referee_list);

      static void OnRefereeChanged (Player    *referee,
                                    Attribute *attr,
                                    Object    *owner,
                                    guint      step);

      void OnDragDataGet (GtkWidget        *widget,
                          GdkDragContext   *drag_context,
                          GtkSelectionData *data,
                          guint             key,
                          guint             time) override;

      void DrawContainerPage (GtkPrintOperation *operation,
                              GtkPrintContext   *context,
                              gint               page_nr) override;

      void CellDataFunc (GtkTreeViewColumn *tree_column,
                         GtkCellRenderer   *cell,
                         GtkTreeModel      *tree_model,
                         GtkTreeIter       *iter) override;
  };
}
