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

#include "player.hpp"

#include "attribute.hpp"
#include "players_base.hpp"

// --------------------------------------------------------------------------------
PlayersBase_c::PlayersBase_c ()
{
  _glade = new Glade_c ("player.glade");

  _player_list = NULL;

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

  {
    _glade->Bind ("add_button",   this);
    _glade->Bind ("close_button", this);
  }
}

// --------------------------------------------------------------------------------
PlayersBase_c::~PlayersBase_c ()
{
  for (guint i = 0; i <  g_slist_length (_player_list); i++)
  {
    Player_c *p;

    p = GetPlayer (i);
    p->Release ();
  }

  g_slist_free (_player_list);
  Object_c::Release (_glade);
}

// --------------------------------------------------------------------------------
GtkTreeModel *PlayersBase_c::GetTreeModel ()
{
  return GTK_TREE_MODEL (_store);
}

// --------------------------------------------------------------------------------
GtkTreeRowReference *PlayersBase_c::GetPlayerRowRef (GtkTreeIter *iter)
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
void PlayersBase_c::EnterPlayers ()
{
  GtkWidget *w = _glade->GetRootWidget ();

  if (w == NULL) return;
  gtk_widget_show_all (w);
}

// --------------------------------------------------------------------------------
void PlayersBase_c::RemoveSelection (GtkTreeSelection *selection)
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

      p = GetPlayer (i);
      current_path = gtk_tree_row_reference_get_path ((GtkTreeRowReference *) p->GetData ("PlayersBase_c::tree_row_ref"));

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
extern "C" G_MODULE_EXPORT void on_close_button_clicked (GtkWidget *widget,
                                                         GdkEvent  *event,
                                                         gpointer  *data)
{
  PlayersBase_c *pb = (PlayersBase_c *) g_object_get_data (G_OBJECT (widget),
                                                           "instance");
  pb->on_close_button_clicked ();
}

// --------------------------------------------------------------------------------
void PlayersBase_c::on_close_button_clicked ()
{
  GtkWidget *root = _glade->GetRootWidget ();

  gtk_widget_hide (root);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_add_button_clicked (GtkWidget *widget,
                                                       GdkEvent  *event,
                                                       gpointer  *data)
{
  PlayersBase_c *pb = (PlayersBase_c *) g_object_get_data (G_OBJECT (widget),
                                                           "instance");
  pb->on_add_button_clicked ();
}

// --------------------------------------------------------------------------------
void PlayersBase_c::on_add_button_clicked ()
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

  player->SetData ("PlayersBase_c::tree_row_ref",
                   GetPlayerRowRef (&iter),
                   (GDestroyNotify) gtk_tree_row_reference_free);

  _player_list = g_slist_append (_player_list,
                                 player);

  Update (player);
}

// --------------------------------------------------------------------------------
guint PlayersBase_c::GetNbPlayers ()
{
  return  g_slist_length (_player_list);
}

// --------------------------------------------------------------------------------
Player_c *PlayersBase_c::GetPlayerFromRef (guint ref)
{
  for (guint i = 0; i < g_slist_length (_player_list); i++)
  {
    Player_c *p;

    p = (Player_c *) g_slist_nth_data (_player_list, i);

    if (p->GetRef () == ref)
    {
      return p;
    }
  }
  return NULL;
}

// --------------------------------------------------------------------------------
Player_c *PlayersBase_c::GetPlayer (guint index)
{
  return (Player_c *) g_slist_nth_data (_player_list, index);
}

// --------------------------------------------------------------------------------
void PlayersBase_c::Load (xmlDoc *doc)
{
  xmlXPathContext *xml_context = xmlXPathNewContext (doc);
  xmlXPathObject  *xml_object;
  xmlNodeSet      *xml_nodeset;

  xml_object  = xmlXPathEval (BAD_CAST "/contest/player_list/*", xml_context);
  xml_nodeset = xml_object->nodesetval;

  for (gint i = 0; i < xml_object->nodesetval->nodeNr; i++)
  {
    Player_c    *player = new Player_c;
    GtkTreeIter  iter;

    gtk_list_store_append (_store, &iter);

    player->Load (xml_nodeset->nodeTab[i]);

    player->SetData ("PlayersBase_c::tree_row_ref",
                     GetPlayerRowRef (&iter),
                     (GDestroyNotify) gtk_tree_row_reference_free);

    _player_list = g_slist_append (_player_list,
                                   player);

    Update (player);
  }

  xmlXPathFreeObject  (xml_object);
  xmlXPathFreeContext (xml_context);
}

// --------------------------------------------------------------------------------
void PlayersBase_c::Save (xmlTextWriter *xml_writer)
{
  int result;

  result = xmlTextWriterStartElement (xml_writer,
                                      BAD_CAST "player_list");

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
void PlayersBase_c::Update (Player_c *player)
{
  GtkTreeRowReference *ref;
  GtkTreePath         *path;
  GtkTreeIter          iter;

  ref  = (GtkTreeRowReference *) player->GetData ("PlayersBase_c::tree_row_ref");
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
Player_c *PlayersBase_c::GetPlayer (const gchar *path_string)
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
    current_ref = (GtkTreeRowReference *) p->GetData ("PlayersBase_c::tree_row_ref");
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
void PlayersBase_c::Lock ()
{
  _player_list = g_slist_sort_with_data (_player_list,
                                         (GCompareDataFunc) Player_c::Compare,
                                         (void *) "rating");

  for (guint i = 0; i <  g_slist_length (_player_list); i++)
  {
    Player_c *p;

    p = GetPlayer (i);
    p->SetAttributeValue ("rank",
                          i + 1);
    Update (p);
  }
}

// --------------------------------------------------------------------------------
GSList *PlayersBase_c::CreateCustomList (CustomFilter filter)
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

  if (custom_list)
  {
    custom_list = g_slist_sort_with_data (custom_list,
                                          (GCompareDataFunc) Player_c::Compare,
                                          (void *) "rank");
  }

  return custom_list;
}
