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

#include "schedule.hpp"

#include "players_list.hpp"

// --------------------------------------------------------------------------------
PlayersList_c::PlayersList_c (gchar         *name,
                              PlayersBase_c *players_base)
: Stage_c (name),
  Module_c ("players_list.glade")
{
  _players_base = players_base;

  // Sensitive widgets
  {
    AddSensitiveWidget (_glade->GetWidget ("add_player_button"));
    AddSensitiveWidget (_glade->GetWidget ("remove_player_button"));
  }

  // Callbacks binding
  {
    _glade->Bind ("add_player_button",    this);
    _glade->Bind ("remove_player_button", this);
  }
}

// --------------------------------------------------------------------------------
PlayersList_c::~PlayersList_c ()
{
}

// --------------------------------------------------------------------------------
void PlayersList_c::on_cell_edited (GtkCellRendererText *cell,
                                    gchar               *path_string,
                                    gchar               *new_text,
                                    gpointer             user_data)
{
  PlayersList_c *c = (PlayersList_c *) user_data;
  gchar *attr_name = (gchar *) g_object_get_data (G_OBJECT (cell),
                                                  "PlayersList_c::attribute_name");

  c->OnCellEdited (path_string,
                   new_text,
                   attr_name);
}

// --------------------------------------------------------------------------------
void PlayersList_c::on_cell_toggled (GtkCellRendererToggle *cell,
                                     gchar                 *path_string,
                                     gpointer               user_data)
{
  PlayersList_c *c = (PlayersList_c *) user_data;
  gchar *attr_name = (gchar *) g_object_get_data (G_OBJECT (cell),
                                                  "PlayersList_c::attribute_name");

  c->OnCellToggled (path_string,
                    gtk_cell_renderer_toggle_get_active (cell),
                    attr_name);
}

// --------------------------------------------------------------------------------
void PlayersList_c::OnCellEdited (gchar *path_string,
                                  gchar *new_text,
                                  gchar *attr_name)
{
  Player_c *p = _players_base->GetPlayer (path_string);

  p->SetAttributeValue (attr_name,
                        new_text);

  _players_base->Update (p);
}

// --------------------------------------------------------------------------------
void PlayersList_c::OnCellToggled (gchar    *path_string,
                                   gboolean  is_active,
                                   gchar    *attr_name)
{
  Player_c *p = _players_base->GetPlayer (path_string);

  if (is_active)
  {
    p->SetAttributeValue (attr_name,
                          "0");
  }
  else
  {
    p->SetAttributeValue (attr_name,
                          "1");
  }

  _players_base->Update (p);
}

// --------------------------------------------------------------------------------
void PlayersList_c::SetColumn (guint     id,
                               gchar    *attr_name,
                               gboolean  entry_is_text_based)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer   *renderer;

  if (entry_is_text_based)
  {
    renderer = gtk_cell_renderer_text_new ();

    g_object_set (renderer,
                  "editable", TRUE,
                  NULL);
    g_signal_connect (renderer,
                      "edited", G_CALLBACK (on_cell_edited), this);

    column = gtk_tree_view_column_new_with_attributes (attr_name,
                                                       renderer,
                                                       "text", id,
                                                       0,      NULL);
    g_object_set_data (G_OBJECT (renderer),
                       "PlayersList_c::SensitiveAttribute",
                       (void *) "editable");
  }
  else
  {
    renderer = gtk_cell_renderer_toggle_new ();

    g_object_set (renderer,
                  "activatable", TRUE,
                  NULL);
    g_signal_connect (renderer,
                      "toggled", G_CALLBACK (on_cell_toggled), this);
    column = gtk_tree_view_column_new_with_attributes (attr_name,
                                                       renderer,
                                                       "active", id,
                                                       0,        NULL);
    g_object_set_data (G_OBJECT (renderer),
                       "PlayersList_c::SensitiveAttribute",
                       (void *) "activatable");
  }

  g_object_set_data (G_OBJECT (renderer),
                     "PlayersList_c::attribute_name", attr_name);

  gtk_tree_view_column_set_sort_column_id (column,
                                           id);

  gtk_tree_view_append_column (GTK_TREE_VIEW (_tree_view),
                               column);
}

// --------------------------------------------------------------------------------
void PlayersList_c::OnPlugged ()
{
  // players tree view
  {
    _tree_view = _glade->GetWidget ("players_list");

    g_object_set (_tree_view,
                  "rules-hint",     TRUE,
                  "rubber-banding", TRUE,
                  NULL);

    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (_tree_view)),
                                 GTK_SELECTION_MULTIPLE);

    gtk_tree_view_set_model (GTK_TREE_VIEW (_tree_view),
                             _players_base->GetTreeModel ());

    for (guint i = 0; i < Attribute_c::GetNbAttributes (); i++)
    {
      gchar *name;
      GType  type;

      Attribute_c::GetNthAttribute (i,
                                    &name,
                                    &type);
      SetColumn (i,
                 name,
                 type != G_TYPE_BOOLEAN);
    }
  }
}

// --------------------------------------------------------------------------------
void PlayersList_c::Load (xmlDoc *doc)
{
}

// --------------------------------------------------------------------------------
void PlayersList_c::Save (xmlTextWriter *xml_writer)
{
}

// --------------------------------------------------------------------------------
void PlayersList_c::Enter ()
{
  SetSensitiveState (TRUE);
}

// --------------------------------------------------------------------------------
void PlayersList_c::OnLocked ()
{
  DisableSensitiveWidgets ();
  SetSensitiveState (FALSE);
}

// --------------------------------------------------------------------------------
void PlayersList_c::OnUnLocked ()
{
  EnableSensitiveWidgets ();
  SetSensitiveState (TRUE);
}

// --------------------------------------------------------------------------------
void PlayersList_c::Cancel ()
{
}

// --------------------------------------------------------------------------------
void PlayersList_c::SetSensitiveState (bool sensitive_value)
{
  GList *columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (_tree_view));

  for (guint c = 0; c < g_list_length (columns) ; c++)
  {
    GList             *renderers;
    GtkTreeViewColumn *column;

    column = GTK_TREE_VIEW_COLUMN (g_list_nth_data (columns, c));
    renderers = gtk_tree_view_column_get_cell_renderers (column);

    for (guint r = 0; r < g_list_length (renderers) ; r++)
    {
      GtkCellRenderer *renderer;
      gchar           *sensitive_attribute;

      renderer = GTK_CELL_RENDERER (g_list_nth_data (renderers, r));

      sensitive_attribute = (gchar *) g_object_get_data (G_OBJECT (renderer),
                                                         "PlayersList_c::SensitiveAttribute");
      g_object_set (renderer,
                    sensitive_attribute, sensitive_value,
                    NULL);
    }

    g_list_free (renderers);
  }

  g_list_free (columns);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_add_player_button_clicked (GtkWidget *widget,
                                                              GdkEvent  *event,
                                                              gpointer  *data)
{
  PlayersList_c *c = (PlayersList_c *) g_object_get_data (G_OBJECT (widget),
                                                          "instance");
  c->on_add_player_button_clicked ();
}

// --------------------------------------------------------------------------------
void PlayersList_c::on_add_player_button_clicked ()
{
  _players_base->EnterPlayers ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_remove_player_button_clicked (GtkWidget *widget,
                                                                 GdkEvent  *event,
                                                                 gpointer  *data)
{
  PlayersList_c *c = (PlayersList_c *) g_object_get_data (G_OBJECT (widget),
                                                          "instance");
  c->on_remove_player_button_clicked ();
}

// --------------------------------------------------------------------------------
void PlayersList_c::on_remove_player_button_clicked ()
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (_tree_view));

  _players_base->RemoveSelection (selection);
}
