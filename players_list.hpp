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
#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>

#include "module.hpp"
#include "attribute.hpp"
#include "stage.hpp"

class PlayersList_c : public virtual Stage_c, public Module_c
{
  public:
    static void Init ();

    PlayersList_c (StageClass *stage_class);

  public:
    void on_add_player_button_clicked ();
    void on_remove_player_button_clicked ();
    void on_add_button_clicked ();
    void on_close_button_clicked ();

    void Import ();

  private:
    void OnLocked ();
    void OnUnLocked ();
    void Wipe ();

  private:
    typedef gboolean (*CustomFilter) (Player_c *player);

    static const gchar *_class_name;
    static const gchar *_xml_class_name;

    GtkWidget    *_tree_view;
    GtkListStore *_store;
    GSList       *_player_list;

    static Stage_c *CreateInstance (StageClass *stage_class);

    void SetColumn (guint     id,
                    gchar    *attr,
                    gboolean  entry_is_text_based,
                    gint      at);

    void Load (xmlNode *xml_node);

    void Save (xmlTextWriter *xml_writer);

    void ImportFFF (gchar *file);

    void ImportCSV (gchar *file);

    void Add (Player_c *player);

    void RemoveSelection (GtkTreeSelection *selection);

    void OnAttrListUpdated ();

    void SetSensitiveState (bool sensitive_value);

    void OnPlugged ();

    void Display ();

    void Update (Player_c *player);

    GSList *CreateCustomList (CustomFilter filter);

    gboolean IsOver ();

    GtkTreeRowReference *GetPlayerRowRef (GtkTreeIter *iter);

    Player_c *GetPlayer (const gchar *path_string);

    static gboolean PresentPlayerFilter (Player_c *player);

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

    ~PlayersList_c ();
};

#endif
