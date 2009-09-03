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

// --------------------------------------------------------------------------------
Module_c::Module_c (gchar *glade_file,
                    gchar *root)
{
  _plugged_list      = NULL;
  _owner             = NULL;
  _sensitive_widgets = NULL;
  _root              = NULL;
  _glade             = NULL;
  _toolbar           = NULL;
  _inserted_ref      = NULL;

  if (glade_file)
  {
    _glade = new Glade_c (glade_file);

    if (root)
    {
      _root = _glade->GetWidget (root);
    }
    else
    {
      _root = _glade->GetRootWidget ();
    }

    _glade->DetachFromParent (_root);
  }

  {
    GtkListStore  *store;
    GtkTreeIter    iter;

    store = gtk_list_store_new (NUM_COLS, G_TYPE_UINT, G_TYPE_STRING);

    for (guint i = 0; i < Attribute_c::GetNbAttributes (); i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          ATTR_NAME, Attribute_c::GetNthAttributeName (i),
                          ATTR_VISIBILITY, FALSE,
                          -1);
    }

    g_signal_connect (G_OBJECT (store),
                      "row-deleted", G_CALLBACK (OnAttrDeleted), this);
    g_signal_connect (G_OBJECT (store),
                      "row-inserted", G_CALLBACK (OnAttrInserted), this);

    _attr_filter_store = GTK_TREE_MODEL (store);
  }
}

// --------------------------------------------------------------------------------
Module_c::~Module_c ()
{
  while (_plugged_list)
  {
    Module_c *module;

    module = (Module_c *) (g_slist_nth_data (_plugged_list, 0)),
    module->UnPlug ();
  }

  if (_plugged_list)
  {
    g_slist_free (_plugged_list);
  }

  UnPlug ();

  if (_sensitive_widgets)
  {
    g_slist_free (_sensitive_widgets);
  }

  Object_c::Release (_glade);
  g_object_unref (_root);

  g_object_unref (_attr_filter_store);
}

// --------------------------------------------------------------------------------
void Module_c::OnAttrDeleted (GtkTreeModel *tree_model,
                              GtkTreePath  *path,
                              gpointer      user_data)
{
  Module_c    *m = (Module_c *) user_data;
  GtkTreeIter  iter;
  gchar       *attr_name;
  bool         visibility;

  {
    GtkTreePath *inserted_path;

    inserted_path = gtk_tree_row_reference_get_path (m->_inserted_ref);
    gtk_tree_model_get_iter (GTK_TREE_MODEL (m->_attr_filter_store),
                             &iter,
                             inserted_path);
    gtk_tree_path_free (inserted_path);
    gtk_tree_row_reference_free (m->_inserted_ref);
    m->_inserted_ref = NULL;
  }

  gtk_tree_model_get (GTK_TREE_MODEL (m->_attr_filter_store),
                      &iter,
                      ATTR_VISIBILITY, &visibility,
                      ATTR_NAME, &attr_name,
                      -1);

  if (visibility == true)
  {
    bool  iter_is_valid;
    guint index = 0;

    m->OnAttrHidden (attr_name);

    iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (m->_attr_filter_store),
                                                   &iter);
    while (iter_is_valid)
    {
      gchar *current_name;
      bool   current_visibility;

      gtk_tree_model_get (GTK_TREE_MODEL (m->_attr_filter_store),
                          &iter,
                          ATTR_VISIBILITY, &current_visibility,
                          ATTR_NAME, &current_name,
                          -1);

      if (strcmp (current_name, attr_name) == 0)
      {
        m->OnAttrShown (attr_name,
                        index);
        iter_is_valid = false;
      }
      else
      {

        if (current_visibility == true)
        {
          index++;
        }

        iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (m->_attr_filter_store),
                                                  &iter);
      }
      g_free (current_name);
    }
  }
  g_free (attr_name);
}

// --------------------------------------------------------------------------------
void Module_c::OnAttrInserted (GtkTreeModel *tree_model,
                               GtkTreePath  *path,
                               GtkTreeIter  *iter,
                               gpointer      user_data)
{
  Module_c *m = (Module_c *) user_data;

  m->_inserted_ref = gtk_tree_row_reference_new (GTK_TREE_MODEL (m->_attr_filter_store),
                                                 path);
}

// --------------------------------------------------------------------------------
void Module_c::SelectAttributes ()
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  {
    GtkCellRenderer *renderer;

    _attr_filter_view = gtk_tree_view_new ();

    g_object_set (_attr_filter_view,
                  "reorderable", TRUE,
                  "headers-visible", FALSE,
                  NULL);

    renderer = gtk_cell_renderer_toggle_new ();
    g_object_set (renderer,
                  "activatable", TRUE,
                  NULL);
    g_signal_connect (renderer,
                      "toggled", G_CALLBACK (on_cell_toggled), this);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (_attr_filter_view),
                                                 -1,
                                                 "Visibility",
                                                 renderer,
                                                 "active", ATTR_VISIBILITY,
                                                 NULL);

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (_attr_filter_view),
                                                 -1,
                                                 "Name",
                                                 renderer,
                                                 "text", ATTR_NAME,
                                                 NULL);

    gtk_tree_view_set_model (GTK_TREE_VIEW (_attr_filter_view), _attr_filter_store);
    gtk_container_add (GTK_CONTAINER (window), _attr_filter_view);
  }

  gtk_widget_show_all (window);
}

// --------------------------------------------------------------------------------
void Module_c::on_cell_toggled (GtkCellRendererToggle *cell,
                                gchar                 *path_string,
                                gpointer               user_data)
{
  Module_c *c = (Module_c *) user_data;

  c->OnCellToggled (path_string,
                    gtk_cell_renderer_toggle_get_active (cell));
}

// --------------------------------------------------------------------------------
void Module_c::OnCellToggled (gchar    *path_string,
                              gboolean  is_active)
{
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter  iter;
  gchar       *attr_name;

  gtk_tree_model_get_iter (GTK_TREE_MODEL (_attr_filter_store),
                           &iter,
                           path);
  gtk_tree_path_free (path);

  gtk_list_store_set (GTK_LIST_STORE (_attr_filter_store),
                      &iter,
                      ATTR_VISIBILITY, !is_active, -1);

  gtk_tree_model_get (GTK_TREE_MODEL (_attr_filter_store),
                      &iter,
                      ATTR_NAME, &attr_name,
                      -1);

  if (is_active)
  {
    OnAttrHidden (attr_name);
  }
  else
  {
    GtkTreeIter iter;
    bool        iter_is_valid;
    guint       index = 0;

    iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_attr_filter_store),
                                                   &iter);

    while (iter_is_valid)
    {
      gchar *current_name;
      bool   current_visibility;

      gtk_tree_model_get (GTK_TREE_MODEL (_attr_filter_store),
                          &iter,
                          ATTR_VISIBILITY, &current_visibility,
                          ATTR_NAME, &current_name,
                          -1);

      if (strcmp (current_name, attr_name) == 0)
      {
        OnAttrShown (attr_name,
                     index);
        iter_is_valid = false;
      }
      else
      {
        if (current_visibility == true)
        {
          index++;
        }

        iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_attr_filter_store),
                                                  &iter);
      }
      g_free (current_name);
    }
  }
  g_free (attr_name);
}

// --------------------------------------------------------------------------------
void Module_c::ShowAttribute (gchar *name)
{
  GtkTreeIter iter;
  bool        iter_is_valid;

  iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_attr_filter_store),
                                                 &iter);

  while (iter_is_valid)
  {
    gchar *current_name;

    gtk_tree_model_get (GTK_TREE_MODEL (_attr_filter_store),
                        &iter,
                        ATTR_NAME, &current_name,
                        -1);
    if (strcmp (current_name, name) == 0)
    {
      gtk_list_store_set (GTK_LIST_STORE (_attr_filter_store),
                          &iter,
                          ATTR_VISIBILITY, TRUE, -1);
    }

    iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_attr_filter_store),
                                              &iter);
  }
}

// --------------------------------------------------------------------------------
void Module_c::Plug (Module_c   *module,
                     GtkWidget  *in,
                     GtkToolbar *toolbar)
{
  gtk_container_add (GTK_CONTAINER (in),
                     module->_root);

  module->_toolbar = toolbar;

  module->OnPlugged ();

  {
    GtkTreeIter iter;
    bool        iter_is_valid;
    guint       nb_visible = 0;

    iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (module->_attr_filter_store),
                                                   &iter);

    while (iter_is_valid)
    {
      gchar *current_name;
      bool   current_visibility;

      gtk_tree_model_get (GTK_TREE_MODEL (module->_attr_filter_store),
                          &iter,
                          ATTR_VISIBILITY, &current_visibility,
                          ATTR_NAME, &current_name,
                          -1);

      if (current_visibility == true)
      {
        module->OnAttrShown (current_name,
                             nb_visible);
        nb_visible++;
      }

      iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (module->_attr_filter_store),
                                                &iter);
    }
  }

  _plugged_list = g_slist_append (_plugged_list,
                                  module);
  module->_owner = this;
}

// --------------------------------------------------------------------------------
void Module_c::UnPlug ()
{
  if (_root)
  {
    GtkWidget *parent = gtk_widget_get_parent (_root);

    if (parent)
    {
      gtk_container_remove (GTK_CONTAINER (parent),
                            _root);
    }

    if (_owner)
    {
      _owner->_plugged_list = g_slist_remove (_owner->_plugged_list,
                                              this);
      _owner = NULL;
    }

    OnUnPlugged ();
  }
}

// --------------------------------------------------------------------------------
void Module_c::OnPlugged ()
{
}

// --------------------------------------------------------------------------------
void Module_c::OnUnPlugged ()
{
}

// --------------------------------------------------------------------------------
GtkWidget *Module_c::GetRootWidget ()
{
  return _root;
}

// --------------------------------------------------------------------------------
GtkToolbar *Module_c::GetToolbar ()
{
  return _toolbar;
}

// --------------------------------------------------------------------------------
void Module_c::AddSensitiveWidget (GtkWidget *w)
{
  _sensitive_widgets = g_slist_append (_sensitive_widgets,
                                       w);
}

// --------------------------------------------------------------------------------
void Module_c::EnableSensitiveWidgets ()
{
  for (guint i = 0; i < g_slist_length (_sensitive_widgets); i++)
  {
    gtk_widget_set_sensitive (GTK_WIDGET (g_slist_nth_data (_sensitive_widgets, i)),
                              true);
  }
}

// --------------------------------------------------------------------------------
void Module_c::DisableSensitiveWidgets ()
{
  for (guint i = 0; i < g_slist_length (_sensitive_widgets); i++)
  {
    gtk_widget_set_sensitive (GTK_WIDGET (g_slist_nth_data (_sensitive_widgets, i)),
                              false);
  }
}
