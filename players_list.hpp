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

class PlayersList : public Module_c
{
  public:
    void Add (Player_c *player);

    void Wipe ();

  protected:
    static const guint NO_RIGHT   = 0x00000000;
    static const guint SORTABLE   = 0x00000001;
    static const guint MODIFIABLE = 0x00000002;

    GSList *_player_list;

    typedef gboolean (*CustomFilter) (Player_c *player);

    PlayersList (gchar *glade_file,
                 guint  rights = SORTABLE | MODIFIABLE);

    ~PlayersList ();

    void RemoveSelection ();

    void OnAttrListUpdated ();

    void SetSensitiveState (bool sensitive_value);

    void Update (Player_c *player);

    GSList *CreateCustomList (CustomFilter filter);

    GSList *GetSelectedPlayers ();

    void SetFilter (Filter *filter);

  private:
    GtkWidget    *_tree_view;
    GtkListStore *_store;
    guint         _rights;

    void SetColumn (guint          id,
                    AttributeDesc *desc,
                    gint           at);

    GtkTreeRowReference *GetPlayerRowRef (GtkTreeIter *iter);

    Player_c *GetPlayer (const gchar *path_string);

    void OnCellEdited (gchar *path_string,
                       gchar *new_text,
                       gchar *attr_name);

    void OnCellToggled (gchar    *path_string,
                        gboolean  is_active,
                        gchar    *attr_name);

    virtual void OnPlayerRemoved (Player_c *player) {};

    static void on_cell_toggled (GtkCellRendererToggle *cell,
                                 gchar                 *path_string,
                                 gpointer               user_data);

    static void on_cell_edited (GtkCellRendererText *cell,
                                gchar               *path_string,
                                gchar               *new_text,
                                gpointer             user_data);

    static gint CompareIterator (GtkTreeModel *model,
                                 GtkTreeIter  *a,
                                 GtkTreeIter  *b,
                                 gchar        *attr_name);
};

#endif
