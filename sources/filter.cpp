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

#include <stdlib.h>
#include <string.h>

#include "attribute.hpp"
#include "module.hpp"

#include "filter.hpp"

// --------------------------------------------------------------------------------
Filter::Filter (GSList *attr_list,
                Module *owner)
: Object ("Filter")
{
  _filter_window = NULL;
  _selected_attr = NULL;
  _attr_list     = attr_list;
  _owner         = owner;

  {
    GtkListStore  *store;
    GtkTreeIter    iter;

    store = gtk_list_store_new (NUM_COLS, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING);

    for (guint i = 0; i < g_slist_length (_attr_list); i++)
    {
      AttributeDesc *desc;

      desc = (AttributeDesc *) g_slist_nth_data (_attr_list,
                                                 i);
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          ATTR_USER_NAME, desc->_user_name,
                          ATTR_XML_NAME, desc->_xml_name,
                          ATTR_VISIBILITY, FALSE,
                          -1);
    }

    g_signal_connect (G_OBJECT (store),
                      "row-deleted", G_CALLBACK (OnAttrDeleted), this);

    _attr_filter_store = store;
  }
}

// --------------------------------------------------------------------------------
Filter::~Filter ()
{
  if (_filter_window)
  {
    gtk_object_destroy (GTK_OBJECT (_filter_window));
  }

  g_slist_free (_selected_attr);

  g_slist_free (_attr_list);

  g_object_unref (_attr_filter_store);
}

// --------------------------------------------------------------------------------
void Filter::SetOwner (Module *owner)
{
  _owner = owner;
}

// --------------------------------------------------------------------------------
guint Filter::GetAttributeId (gchar *name)
{
  if (_attr_list)
  {
    for (guint i = 0; i < g_slist_length (_attr_list); i++)
    {
      AttributeDesc *desc;

      desc = (AttributeDesc *) g_slist_nth_data (_attr_list, i);
      if (strcmp (desc->_xml_name, name) == 0)
      {
        return i;
      }
    }
  }

  return 0xFFFFFFFF;
}

// --------------------------------------------------------------------------------
GSList *Filter::GetAttrList ()
{
  return _attr_list;
}

// --------------------------------------------------------------------------------
GSList *Filter::GetSelectedAttrList ()
{
  return _selected_attr;
}

// --------------------------------------------------------------------------------
void Filter::SetAttributeList (GSList *list)
{
}

// --------------------------------------------------------------------------------
void Filter::ShowAttribute (gchar *name)
{
  GtkTreeIter  iter;
  GtkTreeIter *sibling = NULL;
  gboolean     iter_is_valid;

  iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_attr_filter_store),
                                                 &iter);

  while (iter_is_valid)
  {
    gboolean   current_visibility;
    gchar *current_name;

    gtk_tree_model_get (GTK_TREE_MODEL (_attr_filter_store),
                        &iter,
                        ATTR_VISIBILITY, &current_visibility,
                        ATTR_XML_NAME, &current_name,
                        -1);

    if (current_visibility == TRUE)
    {
      if (sibling)
      {
        gtk_tree_iter_free (sibling);
      }
      sibling = gtk_tree_iter_copy (&iter);
    }

    if (strcmp (current_name, name) == 0)
    {
      gtk_list_store_set (GTK_LIST_STORE (_attr_filter_store),
                          &iter,
                          ATTR_VISIBILITY, TRUE, -1);
      gtk_list_store_move_before (GTK_LIST_STORE (_attr_filter_store),
                                  &iter,
                                  sibling);

      _selected_attr = g_slist_append (_selected_attr,
                                       AttributeDesc::GetDesc (current_name));
      break;
    }

    iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_attr_filter_store),
                                              &iter);
  }

  if (sibling)
  {
    gtk_tree_iter_free (sibling);
  }
}

// --------------------------------------------------------------------------------
void Filter::SelectAttributes ()
{
  if (_filter_window == NULL)
  {
    _filter_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    g_object_set (_filter_window,
                  "transient-for", gtk_widget_get_parent_window (_filter_window),
                  "destroy-with-parent", TRUE,
                  "title", "Filtre",
                  NULL);
    g_signal_connect (_filter_window,
                      "delete-event", G_CALLBACK (gtk_widget_hide_on_delete), NULL);

    {
      GtkCellRenderer *renderer;
      GtkWidget       *view     = gtk_tree_view_new ();

      g_object_set (view,
                    "reorderable", TRUE,
                    "headers-visible", FALSE,
                    NULL);

      renderer = gtk_cell_renderer_toggle_new ();
      g_object_set (renderer,
                    "activatable", TRUE,
                    NULL);
      g_signal_connect (renderer,
                        "toggled", G_CALLBACK (on_cell_toggled), this);
      gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
                                                   -1,
                                                   "Visibility",
                                                   renderer,
                                                   "active", ATTR_VISIBILITY,
                                                   NULL);

      renderer = gtk_cell_renderer_text_new ();
      gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
                                                   -1,
                                                   "Name",
                                                   renderer,
                                                   "text", ATTR_USER_NAME,
                                                   NULL);

      gtk_tree_view_set_model (GTK_TREE_VIEW (view), GTK_TREE_MODEL (_attr_filter_store));
      gtk_container_add (GTK_CONTAINER (_filter_window), view);
    }
  }

  gtk_widget_show_all (_filter_window);
}

// --------------------------------------------------------------------------------
void Filter::UpdateAttrList ()
{
  g_slist_free (_selected_attr);
  _selected_attr = NULL;

  {
    GtkTreeIter iter;
    gboolean    iter_is_valid;

    iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_attr_filter_store),
                                                   &iter);

    while (iter_is_valid)
    {
      gchar     *current_name;
      gboolean   current_visibility;

      gtk_tree_model_get (GTK_TREE_MODEL (_attr_filter_store),
                          &iter,
                          ATTR_VISIBILITY, &current_visibility,
                          ATTR_XML_NAME, &current_name,
                          -1);

      if (current_visibility == TRUE)
      {
        _selected_attr = g_slist_append (_selected_attr,
                                         AttributeDesc::GetDesc (current_name));
      }

      iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_attr_filter_store),
                                                &iter);
    }
  }

  _owner->OnAttrListUpdated ();
}

// --------------------------------------------------------------------------------
void Filter::on_cell_toggled (GtkCellRendererToggle *cell,
                              gchar                 *path_string,
                              gpointer               user_data)
{
  Filter *f = (Filter *) user_data;

  {
    GtkTreePath *path      = gtk_tree_path_new_from_string (path_string);
    gboolean     is_active = gtk_cell_renderer_toggle_get_active (cell);
    GtkTreeIter  iter;

    gtk_tree_model_get_iter (GTK_TREE_MODEL (f->_attr_filter_store),
                             &iter,
                             path);
    gtk_tree_path_free (path);

    gtk_list_store_set (GTK_LIST_STORE (f->_attr_filter_store),
                        &iter,
                        ATTR_VISIBILITY, !is_active, -1);
  }

  f->UpdateAttrList ();
}

// --------------------------------------------------------------------------------
void Filter::OnAttrDeleted (GtkTreeModel *tree_model,
                            GtkTreePath  *path,
                            gpointer      user_data)
{
  Filter *f = (Filter *) user_data;

  f->UpdateAttrList ();
}
