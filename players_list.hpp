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

#ifndef players_list_hpp
#define players_list_hpp

#include <gtk/gtk.h>

#include "module.hpp"
#include "attribute.hpp"
#include "players_base.hpp"

class PlayersList_c : public Module_c
{
  public:
     PlayersList_c (PlayersBase_c *players_base);
    ~PlayersList_c ();

  public:
    void on_add_player_button_clicked    ();
    void on_remove_player_button_clicked ();

  private:
    GtkWidget     *_tree_view;
    PlayersBase_c *_players_base;

    void SetColumn (guint     id,
                    gchar    *attr,
                    gboolean  entry_is_text_based);

    void SetSensitiveState (bool sensitive_value);
    void OnPlugged ();
    void OnCheckInEntered ();
    void OnCheckInLeaved ();
    void OnCellEdited (gchar *path_string,
                       gchar *new_text,
                       gchar *attr_name);
    void OnCellToggled (gchar    *path_string,
                        gboolean  is_active,
                        gchar    *attr_name);

    static void on_cell_toggled (GtkCellRendererToggle *cell,
                                 gchar                 *path_string,
                                 gpointer               user_data);

    static void on_cell_edited (GtkCellRendererText *cell,
                                gchar               *path_string,
                                gchar               *new_text,
                                gpointer             user_data);
};

#endif
