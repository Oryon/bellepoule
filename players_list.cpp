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

#include "schedule.hpp"

#include "players_list.hpp"

const gchar *PlayersList_c::_class_name = "checkin";

// --------------------------------------------------------------------------------
PlayersList_c::PlayersList_c (StageClass *stage_class)
: Stage_c (stage_class),
  Module_c ("players_list.glade")
{
  _players_base = NULL;

  // Sensitive widgets
  {
    AddSensitiveWidget (_glade->GetWidget ("add_player_button"));
    AddSensitiveWidget (_glade->GetWidget ("remove_player_button"));
    AddSensitiveWidget (_glade->GetWidget ("import_toolbutton"));
  }

  // Player attributes to display
  {
    ShowAttribute ("attending");
    ShowAttribute ("rank");
    ShowAttribute ("name");
    ShowAttribute ("first_name");
    ShowAttribute ("club");
    ShowAttribute ("birth_year");
    ShowAttribute ("rating");
    ShowAttribute ("licence");
  }
}

// --------------------------------------------------------------------------------
PlayersList_c::~PlayersList_c ()
{
}

// --------------------------------------------------------------------------------
void PlayersList_c::Init ()
{
  RegisterStageClass (_class_name,
                      CreateInstance,
                      MANDATORY | EDITABLE);
}

// --------------------------------------------------------------------------------
void PlayersList_c::on_cell_edited (GtkCellRendererText *cell,
                                    gchar               *path_string,
                                    gchar               *new_text,
                                    gpointer             user_data)
{
  PlayersList_c *p = (PlayersList_c *) user_data;
  gchar *attr_name = (gchar *) g_object_get_data (G_OBJECT (cell),
                                                  "PlayersList_c::attribute_name");

  p->OnCellEdited (path_string,
                   new_text,
                   attr_name);
}

// --------------------------------------------------------------------------------
void PlayersList_c::on_cell_toggled (GtkCellRendererToggle *cell,
                                     gchar                 *path_string,
                                     gpointer               user_data)
{
  PlayersList_c *p = (PlayersList_c *) user_data;
  gchar *attr_name = (gchar *) g_object_get_data (G_OBJECT (cell),
                                                  "PlayersList_c::attribute_name");

  p->OnCellToggled (path_string,
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
      p = _players_base->GetPlayer (path);
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

        _players_base->Update (p);
      }
    }

    g_list_foreach (selection_list, (GFunc) gtk_tree_path_free, NULL);
    g_list_free    (selection_list);
  }
  else
  {
    Player_c *p = _players_base->GetPlayer (path_string);

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

      _players_base->Update (p);
    }
  }
  gtk_tree_path_free (toggeled_path);
}

// --------------------------------------------------------------------------------
void PlayersList_c::OnAttrListUpdated ()
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
void PlayersList_c::SetColumn (guint     id,
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

  gtk_tree_view_insert_column (GTK_TREE_VIEW (_tree_view),
                               column,
                               at);
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

    gtk_tree_view_set_search_column (GTK_TREE_VIEW (_tree_view),
                                     Attribute_c::GetAttributeId ("name"));
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
  _players_base->Lock ();

  _result = _players_base->CreateCustomList (PresentPlayerFilter);
}

// --------------------------------------------------------------------------------
gboolean PlayersList_c::PresentPlayerFilter (Player_c *player)
{
  Attribute_c *attr = player->GetAttribute ("attending");

  return ((gboolean) attr->GetValue () == TRUE);
}

// --------------------------------------------------------------------------------
void PlayersList_c::OnUnLocked ()
{
  EnableSensitiveWidgets ();
  SetSensitiveState (TRUE);
}

// --------------------------------------------------------------------------------
void PlayersList_c::Wipe ()
{
}

// --------------------------------------------------------------------------------
void PlayersList_c::SetPlayerBase (PlayersBase_c *players_base)
{
  _players_base = players_base;
}

// --------------------------------------------------------------------------------
Stage_c *PlayersList_c::CreateInstance (StageClass *stage_class)
{
  return new PlayersList_c (stage_class);
}

// --------------------------------------------------------------------------------
void PlayersList_c::Import ()
{
  GtkWidget *chooser;

  chooser = GTK_WIDGET (gtk_file_chooser_dialog_new ("Choose a FFE file (.cvs) ...",
                                                     NULL,
                                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                                     GTK_STOCK_CANCEL,
                                                     GTK_RESPONSE_CANCEL,
                                                     GTK_STOCK_OPEN,
                                                     GTK_RESPONSE_ACCEPT,
                                                     NULL));

  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
  {
    gchar *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
    guint  nb_attr  = 0;
    gchar *file;

    {
      gchar *raw_file;
      gsize  length;
      guint  j;

      g_file_get_contents ((const gchar *) filename,
                           &raw_file,
                           &length,
                           NULL);
      length++;

      file = (gchar *) g_malloc (length);

      j = 0;
      for (guint i = 0; i < length; i++)
      {
        if (raw_file[i] != '\r')
        {
          file[j] = raw_file[i];
          j++;
        }
      }
    }

    g_print ("\n\n");
    // Header
    {
      gchar **header_line = g_strsplit_set (file,
                                            "\n",
                                            0);

      if (header_line)
      {
        gchar **header_attr = g_strsplit_set (header_line[0],
                                              ";",
                                              0);

        if (header_attr)
        {
          for (guint i = 0; header_attr[i] != NULL; i++)
          {
            nb_attr++;
          }
          g_strfreev (header_attr);
        }

        g_strfreev (header_line);
      }
    }

    // Fencers
    {
      gchar **tokens = g_strsplit_set (file,
                                       ";\n",
                                       0);

      if (tokens)
      {
        Player_c *player = new Player_c;

        for (guint i = nb_attr; tokens[i] != NULL; i += nb_attr)
        {
          player->SetAttributeValue ("name",       tokens[i]);
          player->SetAttributeValue ("first_name", tokens[i+1]);

          _players_base->Add (player);
          player = new Player_c;
        }
        g_strfreev (tokens);
      }
    }

    g_free (file);
  }

  gtk_widget_destroy (chooser);
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
                                                              Object_c  *owner)
{
  PlayersList_c *p = dynamic_cast <PlayersList_c *> (owner);

  p->on_add_player_button_clicked ();
}

// --------------------------------------------------------------------------------
void PlayersList_c::on_add_player_button_clicked ()
{
  _players_base->EnterPlayers ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_remove_player_button_clicked (GtkWidget *widget,
                                                                 Object_c  *owner)
{
  PlayersList_c *p = dynamic_cast <PlayersList_c *> (owner);

  p->on_remove_player_button_clicked ();
}

// --------------------------------------------------------------------------------
void PlayersList_c::on_remove_player_button_clicked ()
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (_tree_view));

  _players_base->RemoveSelection (selection);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_players_list_filter_button_clicked (GtkWidget *widget,
                                                                       Object_c  *owner)
{
  PlayersList_c *p = dynamic_cast <PlayersList_c *> (owner);

  p->SelectAttributes ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_import_toolbutton_clicked (GtkWidget *widget,
                                                              Object_c  *owner)
{
  PlayersList_c *p = dynamic_cast <PlayersList_c *> (owner);

  p->Import ();
}
