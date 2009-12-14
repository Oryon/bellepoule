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

const gchar *PlayersList_c::_class_name     = "checkin";
const gchar *PlayersList_c::_xml_class_name = "checkin_stage";

// --------------------------------------------------------------------------------
PlayersList_c::PlayersList_c (StageClass *stage_class)
: Stage_c (stage_class),
  Module_c ("players_list.glade")
{
  // Sensitive widgets
  {
    AddSensitiveWidget (_glade->GetWidget ("add_player_button"));
    AddSensitiveWidget (_glade->GetWidget ("remove_player_button"));
    AddSensitiveWidget (_glade->GetWidget ("import_toolbutton"));
  }

  // Player attributes to display
  {
    ShowAttribute ("attending");
    ShowAttribute ("rating");
    ShowAttribute ("name");
    ShowAttribute ("first_name");
    ShowAttribute ("birth_year");
    ShowAttribute ("gender");
    ShowAttribute ("club");
    ShowAttribute ("country");
    ShowAttribute ("licence");
  }

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
PlayersList_c::~PlayersList_c ()
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
void PlayersList_c::Init ()
{
  RegisterStageClass (_class_name,
                      _xml_class_name,
                      CreateInstance,
                      MANDATORY | EDITABLE);
}

// --------------------------------------------------------------------------------
Stage_c *PlayersList_c::CreateInstance (StageClass *stage_class)
{
  return new PlayersList_c (stage_class);
}

// --------------------------------------------------------------------------------
void PlayersList_c::OnPlugged ()
{
}

// --------------------------------------------------------------------------------
void PlayersList_c::Load (xmlNode *xml_node)
{
  for (xmlNode *n = xml_node; n != NULL; n = n->next)
  {
    if (n->type == XML_ELEMENT_NODE)
    {
      if (strcmp ((char *) n->name, "player") == 0)
      {
        Player_c    *player = new Player_c;
        GtkTreeIter  iter;

        gtk_list_store_append (_store, &iter);

        player->Load (n);

        player->SetData ("PlayersList_c::tree_row_ref",
                         GetPlayerRowRef (&iter),
                         (GDestroyNotify) gtk_tree_row_reference_free);

        _player_list = g_slist_append (_player_list,
                                       player);

        Update (player);
      }
      else if (strcmp ((char *) n->name, _xml_class_name) != 0)
      {
        return;
      }
    }
    Load (n->children);
  }
}

// --------------------------------------------------------------------------------
void PlayersList_c::Save (xmlTextWriter *xml_writer)
{
  int result;

  result = xmlTextWriterStartElement (xml_writer,
                                      BAD_CAST _xml_class_name);

  g_print ("    >> %d\n", g_slist_length (_player_list));
  for (guint i = 0; i < g_slist_length (_player_list); i++)
  {
    Player_c *p;

    p = (Player_c *) g_slist_nth_data (_player_list, i);

    if (p)
    {
      p->Save (xml_writer);
    }
  }

  result = xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void PlayersList_c::Display ()
{
  OnAttrListUpdated ();
}

// --------------------------------------------------------------------------------
void PlayersList_c::OnLocked ()
{
  DisableSensitiveWidgets ();
  SetSensitiveState (FALSE);

  // Give a rank to each player
  {
    _player_list = g_slist_sort_with_data (_player_list,
                                           (GCompareDataFunc) Player_c::Compare,
                                           (void *) "rating");

    for (guint i = 0; i <  g_slist_length (_player_list); i++)
    {
      Player_c *p;

      p = (Player_c *) g_slist_nth_data (_player_list, i);
      p->SetAttributeValue ("rank",
                            i + 1);
      Update (p);
    }
  }

  _result = CreateCustomList (PresentPlayerFilter);
  if (_result)
  {
    _result = g_slist_sort_with_data (_result,
                                      (GCompareDataFunc) Player_c::Compare,
                                      (void *) "rank");
  }

}

// --------------------------------------------------------------------------------
void PlayersList_c::Update (Player_c *player)
{
  GtkTreeRowReference *ref;
  GtkTreePath         *path;
  GtkTreeIter          iter;

  ref  = (GtkTreeRowReference *) player->GetData ("PlayersList_c::tree_row_ref");
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
gboolean PlayersList_c::IsOver ()
{
  return TRUE;
}

// --------------------------------------------------------------------------------
GSList *PlayersList_c::CreateCustomList (CustomFilter filter)
{
  GSList *custom_list = NULL;

  g_print (">> %d\n", g_slist_length (_player_list));
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
  Player_c *p = GetPlayer (path_string);

  p->SetAttributeValue (attr_name,
                        new_text);

  Update (p);
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
void PlayersList_c::Import ()
{
  GtkWidget *chooser;

  chooser = GTK_WIDGET (gtk_file_chooser_dialog_new ("Choose a FFE file (.FFF) ...",
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

    if (strstr (filename, ".FFF"))
    {
      ImportFFF (file);
    }
    else
    {
      ImportCSV (file);
    }

    g_free (file);
  }

  gtk_widget_destroy (chooser);
}

// --------------------------------------------------------------------------------
void PlayersList_c::ImportFFF (gchar *file)
{
  gchar **lines = g_strsplit_set (file,
                                  "\n",
                                  0);

  if (lines == NULL)
  {
    return;
  }
  else
  {
    // Header
    // FFF;WIN;CLASSEMENT;FFE
    // 08/11/2009;;;;

    // Fencers
    for (guint l = 2; lines[l] != NULL; l++)
    {
      gchar **tokens = g_strsplit_set (lines[l],
                                       ",;",
                                       0);

      if (tokens && tokens[0])
      {
        Player_c *player = new Player_c;

        player->SetAttributeValue ("name",       tokens[0]);
        player->SetAttributeValue ("first_name", tokens[1]);
        player->SetAttributeValue ("birth_year", tokens[2]);
        player->SetAttributeValue ("gender",     tokens[3]);
        player->SetAttributeValue ("country",    tokens[4]);
        player->SetAttributeValue ("licence",    tokens[8]);
        player->SetAttributeValue ("club",       tokens[10]);
        if (*tokens[11])
        {
          player->SetAttributeValue ("rating",     tokens[11]);
        }
        Add (player);

        g_strfreev (tokens);
      }
    }

    g_strfreev (lines);
  }
}

// --------------------------------------------------------------------------------
void PlayersList_c::ImportCSV (gchar *file)
{
  guint nb_attr = 0;

  gchar **header_line = g_strsplit_set (file,
                                        "\n",
                                        0);

  if (header_line == NULL)
  {
    return;
  }
  else
  {
    // Header
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

          Add (player);
          player = new Player_c;
        }
        g_strfreev (tokens);
      }
    }
  }
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
GtkTreeRowReference *PlayersList_c::GetPlayerRowRef (GtkTreeIter *iter)
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
void PlayersList_c::Add (Player_c *player)
{
  GtkTreeIter iter;

  gtk_list_store_append (_store, &iter);

  {
    gchar *str;

    str = g_strdup_printf ("%d\n", player->GetRef ());
    player->SetAttributeValue ("ref", str);
    g_free (str);

    player->SetAttributeValue ("attending", (guint) FALSE);
  }

  player->SetData ("PlayersList_c::tree_row_ref",
                   GetPlayerRowRef (&iter),
                   (GDestroyNotify) gtk_tree_row_reference_free);

  _player_list = g_slist_append (_player_list,
                                 player);

  Update (player);
}

// --------------------------------------------------------------------------------
void PlayersList_c::RemoveSelection (GtkTreeSelection *selection)
{
  GList *ref_list       = NULL;
  GList *selection_list = NULL;

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
      current_path = gtk_tree_row_reference_get_path ((GtkTreeRowReference *) p->GetData ("PlayersList_c::tree_row_ref"));

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
void PlayersList_c::on_add_button_clicked ()
{
  Player_c    *player = new Player_c;
  GtkTreeIter  iter;

  gtk_list_store_append (_store, &iter);

  {
    gchar           *str;
    gboolean         attending;
    GtkEntry        *entry;
    GtkToggleButton *check_button;

    str = g_strdup_printf ("%d\n", player->GetRef ());
    player->SetAttributeValue ("ref", str);
    g_free (str);

    entry = GTK_ENTRY (_glade->GetWidget ("name_entry"));
    str = (gchar *) gtk_entry_get_text (entry);
    player->SetAttributeValue ("name", str);
    gtk_entry_set_text (entry, "");

    entry = GTK_ENTRY (_glade->GetWidget ("first_name_entry"));
    str = (gchar *) gtk_entry_get_text (entry);
    player->SetAttributeValue ("first_name", str);
    gtk_entry_set_text (entry, "");

    entry = GTK_ENTRY (_glade->GetWidget ("rating_entry"));
    str = (gchar *) gtk_entry_get_text (entry);
    player->SetAttributeValue ("rating", str);
    gtk_entry_set_text (entry, "");

    check_button = GTK_TOGGLE_BUTTON (_glade->GetWidget ("attending_checkbutton"));
    attending = gtk_toggle_button_get_active (check_button);
    player->SetAttributeValue ("attending", attending);
  }

  gtk_widget_grab_focus (_glade->GetWidget ("name_entry"));

  player->SetData ("PlayersList_c::tree_row_ref",
                   GetPlayerRowRef (&iter),
                   (GDestroyNotify) gtk_tree_row_reference_free);

  _player_list = g_slist_append (_player_list,
                                 player);

  Update (player);
}

// --------------------------------------------------------------------------------
Player_c *PlayersList_c::GetPlayer (const gchar *path_string)
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
    current_ref = (GtkTreeRowReference *) p->GetData ("PlayersList_c::tree_row_ref");
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
  GtkWidget *w = _glade->GetWidget ("FillInForm");

  if (w == NULL) return;
  gtk_widget_show_all (w);
}

// --------------------------------------------------------------------------------
void PlayersList_c::on_close_button_clicked ()
{
  GtkWidget *w = _glade->GetWidget ("FillInForm");

  gtk_widget_hide (w);
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

  RemoveSelection (selection);
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

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_add_button_clicked (GtkWidget *widget,
                                                       Object_c  *owner)
{
  PlayersList_c *pl = dynamic_cast <PlayersList_c *> (owner);

  pl->on_add_button_clicked ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_close_button_clicked (GtkWidget *widget,
                                                         Object_c  *owner)
{
  PlayersList_c *pl = dynamic_cast <PlayersList_c *> (owner);

  pl->on_close_button_clicked ();
}
