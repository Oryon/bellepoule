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

#include <string.h>

#include "attribute.hpp"
#include "schedule.hpp"
#include "player.hpp"

#include "players_list.hpp"

// --------------------------------------------------------------------------------
  PlayersList::PlayersList (gchar *glade_file)
: Module_c (glade_file)
{
  {
    guint nb_attr        = Attribute_c::GetNbAttributes ();
    GType types[nb_attr];

    for (guint i = 0; i < nb_attr; i++)
    {
      types[i] = Attribute_c::GetNthAttributeType (i);
    }

    _store = gtk_list_store_newv (nb_attr,
                                  types);
  }

  _player_list = NULL;

  {
    _tree_view = _glade->GetWidget ("players_list");

    g_object_set (_tree_view,
                  "rules-hint",     TRUE,
                  "rubber-banding", TRUE,
                  NULL);

    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (_tree_view)),
                                 GTK_SELECTION_MULTIPLE);

    gtk_tree_view_set_model (GTK_TREE_VIEW (_tree_view),
                             GTK_TREE_MODEL (_store));

    gtk_tree_view_set_search_column (GTK_TREE_VIEW (_tree_view),
                                     Attribute_c::GetAttributeId ("name"));
  }
}

// --------------------------------------------------------------------------------
PlayersList::~PlayersList ()
{
  for (guint i = 0; i <  g_slist_length (_player_list); i++)
  {
    Player_c *p;

    p = (Player_c *) g_slist_nth_data (_player_list, i);
    p->Release ();
  }
  g_slist_free (_player_list);

  gtk_list_store_clear (_store);
  g_object_unref (_store);
}

// --------------------------------------------------------------------------------
void PlayersList::Update (Player_c *player)
{
  GtkTreeRowReference *ref;
  GtkTreePath         *path;
  GtkTreeIter          iter;

  ref  = (GtkTreeRowReference *) player->GetData ("PlayersList::tree_row_ref");
  path = gtk_tree_row_reference_get_path (ref);
  gtk_tree_model_get_iter (GTK_TREE_MODEL (_store),
                           &iter,
                           path);
  gtk_tree_path_free (path);

  for (guint i = 0; i < Attribute_c::GetNbAttributes (); i++)
  {
    Attribute_c *attr;

    attr = player->GetAttribute (Attribute_c::GetNthAttributeName (i));

    if (attr)
    {
      gtk_list_store_set (_store, &iter,
                          i, attr->GetValue (), -1);
    }
  }
}

// --------------------------------------------------------------------------------
GSList *PlayersList::CreateCustomList (CustomFilter filter)
{
  GSList *custom_list = NULL;

  for (guint i = 0; i < g_slist_length (_player_list); i++)
  {
    Player_c *p;

    p = (Player_c *) g_slist_nth_data (_player_list, i);

    if (filter (p) == TRUE)
    {
      custom_list = g_slist_append (custom_list,
                                    p);
    }
  }

  return custom_list;
}

// --------------------------------------------------------------------------------
void PlayersList::on_cell_edited (GtkCellRendererText *cell,
                                  gchar               *path_string,
                                  gchar               *new_text,
                                  gpointer             user_data)
{
  PlayersList *p = (PlayersList *) user_data;
  gchar *attr_name = (gchar *) g_object_get_data (G_OBJECT (cell),
                                                  "PlayersList::attribute_name");

  p->OnCellEdited (path_string,
                   new_text,
                   attr_name);
}

// --------------------------------------------------------------------------------
void PlayersList::on_cell_toggled (GtkCellRendererToggle *cell,
                                   gchar                 *path_string,
                                   gpointer               user_data)
{
  PlayersList *p = (PlayersList *) user_data;
  gchar *attr_name = (gchar *) g_object_get_data (G_OBJECT (cell),
                                                  "PlayersList::attribute_name");

  p->OnCellToggled (path_string,
                    gtk_cell_renderer_toggle_get_active (cell),
                    attr_name);
}

// --------------------------------------------------------------------------------
void PlayersList::OnCellEdited (gchar *path_string,
                                gchar *new_text,
                                gchar *attr_name)
{
  Player_c *p = GetPlayer (path_string);

  p->SetAttributeValue (attr_name,
                        new_text);

  Update (p);
}

// --------------------------------------------------------------------------------
void PlayersList::OnCellToggled (gchar    *path_string,
                                 gboolean  is_active,
                                 gchar    *attr_name)
{
  GtkTreePath      *toggeled_path = gtk_tree_path_new_from_string (path_string);
  GtkTreeSelection *selection     = gtk_tree_view_get_selection (GTK_TREE_VIEW (_tree_view));

  if (gtk_tree_selection_path_is_selected (selection,
                                           toggeled_path))
  {
    GList *selection_list;

    selection_list = gtk_tree_selection_get_selected_rows (selection,
                                                           NULL);

    for (guint i = 0; i < g_list_length (selection_list); i++)
    {
      Player_c *p;
      gchar    *path;

      path = gtk_tree_path_to_string ((GtkTreePath *) g_list_nth_data (selection_list,
                                                                       i));
      p = GetPlayer (path);
      g_free (path);

      if (p)
      {
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

        Update (p);
      }
    }

    g_list_foreach (selection_list, (GFunc) gtk_tree_path_free, NULL);
    g_list_free    (selection_list);
  }
  else
  {
    Player_c *p = GetPlayer (path_string);

    if (p)
    {
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

      Update (p);
    }
  }
  gtk_tree_path_free (toggeled_path);
}

// --------------------------------------------------------------------------------
void PlayersList::OnAttrListUpdated ()
{
  {
    GList *column_list = gtk_tree_view_get_columns (GTK_TREE_VIEW (_tree_view));
    guint  nb_col      = g_list_length (column_list);

    for (guint i = 0; i < nb_col; i++)
    {
      GtkTreeViewColumn *column;

      column = gtk_tree_view_get_column (GTK_TREE_VIEW (_tree_view),
                                         0);
      if (column)
      {
        gtk_tree_view_remove_column (GTK_TREE_VIEW (_tree_view),
                                     column);
      }
    }
  }

  if (_attr_list)
  {
    for (guint i = 0; i < g_slist_length (_attr_list); i++)
    {
      gchar *attr_name;
      GType  type;

      attr_name = (gchar *) g_slist_nth_data (_attr_list,
                                              i);
      type = Attribute_c::GetAttributeType (attr_name);

      SetColumn (Attribute_c::GetAttributeId (attr_name),
                 attr_name,
                 type != G_TYPE_BOOLEAN,
                 -1);
    }
  }
}

// --------------------------------------------------------------------------------
void PlayersList::SetColumn (guint     id,
                             gchar    *attr_name,
                             gboolean  entry_is_text_based,
                             gint      at)
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
                       "PlayersList::SensitiveAttribute",
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
                       "PlayersList::SensitiveAttribute",
                       (void *) "activatable");
  }

  g_object_set_data (G_OBJECT (renderer),
                     "PlayersList::attribute_name", attr_name);

  gtk_tree_view_column_set_sort_column_id (column,
                                           id);

  gtk_tree_view_insert_column (GTK_TREE_VIEW (_tree_view),
                               column,
                               at);
}

// --------------------------------------------------------------------------------
void PlayersList::SetSensitiveState (bool sensitive_value)
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
                                                         "PlayersList::SensitiveAttribute");
      g_object_set (renderer,
                    sensitive_attribute, sensitive_value,
                    NULL);
    }

    g_list_free (renderers);
  }

  g_list_free (columns);
}

// --------------------------------------------------------------------------------
GtkTreeRowReference *PlayersList::GetPlayerRowRef (GtkTreeIter *iter)
{
  GtkTreeRowReference *ref;
  GtkTreePath         *path = gtk_tree_model_get_path (GTK_TREE_MODEL (_store),
                                                       iter);

  ref = gtk_tree_row_reference_new (GTK_TREE_MODEL (_store),
                                    path);

  gtk_tree_path_free (path);

  return ref;
}

// --------------------------------------------------------------------------------
void PlayersList::Add (Player_c *player)
{
  GtkTreeIter iter;

  gtk_list_store_append (_store, &iter);

  {
    gchar *str;

    str = g_strdup_printf ("%d\n", player->GetRef ());
    player->SetAttributeValue ("ref", str);
    g_free (str);
  }

  player->SetData ("PlayersList::tree_row_ref",
                   GetPlayerRowRef (&iter),
                   (GDestroyNotify) gtk_tree_row_reference_free);

  _player_list = g_slist_append (_player_list,
                                 player);

  Update (player);
}

// --------------------------------------------------------------------------------
void PlayersList::RemoveSelection ()
{
  GList            *ref_list       = NULL;
  GList            *selection_list = NULL;
  GtkTreeSelection *selection      = gtk_tree_view_get_selection (GTK_TREE_VIEW (_tree_view));


  selection_list = gtk_tree_selection_get_selected_rows (selection,
                                                         NULL);

  for (guint i = 0; i < g_list_length (selection_list); i++)
  {
    GtkTreeRowReference *ref;

    ref = gtk_tree_row_reference_new (GTK_TREE_MODEL (_store),
                                      (GtkTreePath *) g_list_nth_data (selection_list, i));
    ref_list = g_list_append (ref_list,
                              ref);
  }

  for (guint i = 0; i < g_list_length (ref_list); i++)
  {
    GtkTreeRowReference *ref;
    GtkTreePath         *selected_path;
    GtkTreePath         *current_path;

    ref = (GtkTreeRowReference *) g_list_nth_data (ref_list, i);
    selected_path = gtk_tree_row_reference_get_path (ref);
    gtk_tree_row_reference_free (ref);

    for (guint i = 0; i <  g_slist_length (_player_list); i++)
    {
      Player_c *p;

      p = (Player_c *) g_slist_nth_data (_player_list, i);
      current_path = gtk_tree_row_reference_get_path ((GtkTreeRowReference *) p->GetData ("PlayersList::tree_row_ref"));

      if (gtk_tree_path_compare (selected_path,
                                 current_path) == 0)
      {
        GtkTreeIter iter;

        gtk_tree_model_get_iter (GTK_TREE_MODEL (_store),
                                 &iter,
                                 selected_path);

        gtk_list_store_remove (_store,
                               &iter);

        _player_list = g_slist_remove (_player_list,
                                       p);
        p->Release ();
        break;
      }
      gtk_tree_path_free (current_path);
    }
    gtk_tree_path_free (selected_path);
  }

  g_list_free (ref_list);

  g_list_foreach (selection_list, (GFunc) gtk_tree_path_free, NULL);
  g_list_free    (selection_list);
}

// --------------------------------------------------------------------------------
Player_c *PlayersList::GetPlayer (const gchar *path_string)
{
  GtkTreePath *path;
  Player_c    *result = NULL;

  path = gtk_tree_path_new_from_string (path_string);
  for (guint i = 0; i < g_slist_length (_player_list); i++)
  {
    GtkTreeRowReference *current_ref;
    GtkTreePath         *current_path;
    Player_c            *p;

    p = (Player_c *) g_slist_nth_data (_player_list, i);
    current_ref = (GtkTreeRowReference *) p->GetData ("PlayersList::tree_row_ref");
    current_path = gtk_tree_row_reference_get_path (current_ref);
    if (gtk_tree_path_compare (path,
                               current_path) == 0)
    {
      result = p;
      gtk_tree_path_free (current_path);
      break;
    }

    gtk_tree_path_free (current_path);
  }
  gtk_tree_path_free (path);

  return result;
}
