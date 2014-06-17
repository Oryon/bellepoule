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

static const gchar *look_image[AttributeDesc::NB_LOOK] =
{
  N_ ("Long text"),
  N_ ("Short text"),
  N_ ("Graphic")
};

// --------------------------------------------------------------------------------
Filter::Filter (GSList *attr_list,
                Module *owner)
: Object ("Filter")
{
  _dialog        = NULL;
  _selected_list = NULL;
  _attr_list     = attr_list;
  _owner         = owner;

  {
    GtkTreeIter iter;

    _look_store = gtk_list_store_new (NUM_LOOK_COLS,
                                      G_TYPE_STRING, // LOOK_IMAGE_str
                                      G_TYPE_UINT);  // LOOK_VALUE_uint

    for (guint i = 0; i < AttributeDesc::NB_LOOK; i++)
    {
      gtk_list_store_append (_look_store, &iter);
      gtk_list_store_set (_look_store, &iter,
                          LOOK_IMAGE_str,  gettext (look_image[i]),
                          LOOK_VALUE_uint, i,
                          -1);
    }
  }

  {
    GSList *current = attr_list;

    _attr_filter_store = gtk_list_store_new (NUM_ATTR_COLS,
                                             G_TYPE_BOOLEAN, // ATTR_VISIBILITY_bool
                                             G_TYPE_STRING,  // ATTR_USER_NAME_str
                                             G_TYPE_POINTER, // ATTR_XML_NAME_ptr
                                             G_TYPE_STRING,  // ATTR_LOOK_IMAGE_str
                                             G_TYPE_UINT);   // ATTR_LOOK_VALUE_uint

    while (current)
    {
      GtkTreeIter    iter;
      AttributeDesc *desc = (AttributeDesc *) current->data;

      gtk_list_store_append (_attr_filter_store, &iter);
      gtk_list_store_set (_attr_filter_store, &iter,
                          ATTR_USER_NAME_str,   desc->_user_name,
                          ATTR_XML_NAME_ptr,    desc->_code_name,
                          ATTR_VISIBILITY_bool, FALSE,
                          ATTR_LOOK_IMAGE_str,  gettext (look_image[desc->_favorite_look]),
                          ATTR_LOOK_VALUE_uint, desc->_favorite_look,
                          -1);
      current = g_slist_next (current);
    }

    g_signal_connect (G_OBJECT (_attr_filter_store),
                      "row-deleted", G_CALLBACK (OnAttrDeleted), this);
  }
}

// --------------------------------------------------------------------------------
Filter::~Filter ()
{
  if (_dialog)
  {
    gtk_widget_destroy (GTK_WIDGET (_dialog));
  }

  g_slist_free_full (_selected_list,
                     (GDestroyNotify) Object::TryToRelease);

  g_slist_free   (_attr_list);
  g_object_unref (_attr_filter_store);
  g_object_unref (_look_store);
}

// --------------------------------------------------------------------------------
void Filter::UnPlug ()
{
  if (_dialog)
  {
    // gtk_widget_hide (_dialog);
  }
}

// --------------------------------------------------------------------------------
void Filter::SetOwner (Module *owner)
{
  _owner = owner;
}

// --------------------------------------------------------------------------------
guint Filter::GetAttributeId (const gchar *name)
{
  GSList *current = _attr_list;

  for (guint i = 0; current != NULL; i++)
  {
    AttributeDesc *desc = (AttributeDesc *) current->data;

    if (strcmp (desc->_code_name, name) == 0)
    {
      return i;
    }
    current = g_slist_next (current);
  }

  return 0xFFFFFFFF;
}

// --------------------------------------------------------------------------------
GSList *Filter::GetAttrList ()
{
  return _attr_list;
}

// --------------------------------------------------------------------------------
GSList *Filter::GetLayoutList ()
{
  return _selected_list;
}

// --------------------------------------------------------------------------------
void Filter::ShowAttribute (const gchar *name)
{
  GtkTreeIter  iter;
  GtkTreeIter *sibling = NULL;
  gboolean     iter_is_valid;

  iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_attr_filter_store),
                                                 &iter);

  while (iter_is_valid)
  {
    gboolean  current_visibility;
    gchar    *current_name;

    gtk_tree_model_get (GTK_TREE_MODEL (_attr_filter_store),
                        &iter,
                        ATTR_VISIBILITY_bool, &current_visibility,
                        ATTR_XML_NAME_ptr,    &current_name,
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
      Layout *attr_layout = new Layout ();

      gtk_list_store_set (GTK_LIST_STORE (_attr_filter_store),
                          &iter,
                          ATTR_VISIBILITY_bool, TRUE, -1);
      gtk_list_store_move_before (GTK_LIST_STORE (_attr_filter_store),
                                  &iter,
                                  sibling);

      attr_layout->_desc = AttributeDesc::GetDescFromCodeName (current_name);
      attr_layout->_look = attr_layout->_desc->_favorite_look;
      _selected_list = g_slist_append (_selected_list,
                                       attr_layout);
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
  if (_dialog == NULL)
  {
    _dialog = gtk_dialog_new_with_buttons (gettext ("Data to display"),
                                           NULL,
                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_STOCK_CLOSE,
                                           GTK_RESPONSE_NONE,
                                           NULL);

    g_object_set (_dialog,
                  "transient-for", gtk_widget_get_parent_window (_dialog),
                  "destroy-with-parent", TRUE,
                  "modal", TRUE,
                  NULL);

    {
      GtkWidget *view = gtk_tree_view_new ();

      g_object_set (view,
                    "reorderable", TRUE,
                    NULL);

      // Visibility
      {
        GtkCellRenderer *renderer = gtk_cell_renderer_toggle_new ();

        g_object_set (renderer,
                      "activatable", TRUE,
                      NULL);
        g_signal_connect (renderer,
                          "toggled", G_CALLBACK (OnVisibilityToggled), this);
        gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
                                                     -1,
                                                     gettext ("Visibility"),
                                                     renderer,
                                                     "active", ATTR_VISIBILITY_bool,
                                                     NULL);
      }

      // Name
      {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();

        gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
                                                     -1,
                                                     gettext ("Data"),
                                                     renderer,
                                                     "text", ATTR_USER_NAME_str,
                                                     NULL);
      }

      // Look
      {
        GtkCellRenderer *renderer = gtk_cell_renderer_combo_new ();

        g_object_set (G_OBJECT (renderer),
                      "model",       _look_store,
                      "text-column", LOOK_IMAGE_str,
                      "editable",    TRUE,
                      "has-entry",   FALSE,
                      NULL);
        g_signal_connect (renderer,
                          "changed", G_CALLBACK (OnLookChanged), this);
        gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
                                                     -1,
                                                     gettext ("Look"),
                                                     renderer,
                                                     "text", ATTR_LOOK_IMAGE_str,
                                                     NULL);
      }

      gtk_tree_view_set_model (GTK_TREE_VIEW (view), GTK_TREE_MODEL (_attr_filter_store));

      {
        GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (_dialog));

        gtk_container_add (GTK_CONTAINER (content_area), view);
      }
    }
  }

  gtk_widget_show_all (_dialog);
  gtk_dialog_run (GTK_DIALOG (_dialog));
  gtk_widget_hide (_dialog);
}

// --------------------------------------------------------------------------------
void Filter::UpdateAttrList ()
{
  {
    g_slist_free_full (_selected_list,
                       (GDestroyNotify) Object::TryToRelease);
    _selected_list = NULL;
  }

  {
    GtkTreeIter iter;
    gboolean    iter_is_valid;

    iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_attr_filter_store),
                                                   &iter);

    while (iter_is_valid)
    {
      gchar               *current_name;
      gboolean             current_visibility;
      AttributeDesc::Look  curent_look;

      gtk_tree_model_get (GTK_TREE_MODEL (_attr_filter_store),
                          &iter,
                          ATTR_VISIBILITY_bool, &current_visibility,
                          ATTR_XML_NAME_ptr,    &current_name,
                          ATTR_LOOK_VALUE_uint, &curent_look,
                          -1);

      if (current_visibility == TRUE)
      {
        Layout *attr_layout = new Layout ();

        attr_layout->_look = curent_look;
        attr_layout->_desc = AttributeDesc::GetDescFromCodeName (current_name);
        _selected_list = g_slist_append (_selected_list,
                                         attr_layout);
      }

      iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_attr_filter_store),
                                                &iter);
    }
  }

  if (_owner->IsPlugged ())
  {
    _owner->OnAttrListUpdated ();
  }
}

// --------------------------------------------------------------------------------
void Filter::OnVisibilityToggled (GtkCellRendererToggle *cell,
                                  gchar                 *path_string,
                                  Filter                *filter)
{
  {
    GtkTreePath *path      = gtk_tree_path_new_from_string (path_string);
    gboolean     is_active = gtk_cell_renderer_toggle_get_active (cell);
    gboolean     visibility;
    GtkTreeIter  iter;

    gtk_tree_model_get_iter (GTK_TREE_MODEL (filter->_attr_filter_store),
                             &iter,
                             path);
    gtk_tree_path_free (path);

    if (is_active)
    {
      visibility = FALSE;
    }
    else
    {
      visibility = TRUE;
    }
    gtk_list_store_set (GTK_LIST_STORE (filter->_attr_filter_store),
                        &iter,
                        ATTR_VISIBILITY_bool, visibility,
                        -1);
  }

  filter->UpdateAttrList ();
}

// --------------------------------------------------------------------------------
void Filter::OnLookChanged (GtkCellRendererCombo *combo,
                            gchar                *path_string,
                            GtkTreeIter          *new_iter,
                            Filter               *filter)
{
  gchar *look_image;
  guint  look_value;

  gtk_tree_model_get (GTK_TREE_MODEL (filter->_look_store),
                      new_iter,
                      LOOK_IMAGE_str,  &look_image,
                      LOOK_VALUE_uint, &look_value,
                      -1);

  {
    GtkTreePath *path  = gtk_tree_path_new_from_string (path_string);
    GtkTreeIter  iter;

    gtk_tree_model_get_iter (GTK_TREE_MODEL (filter->_attr_filter_store),
                             &iter,
                             path);
    gtk_tree_path_free (path);

    gtk_list_store_set (GTK_LIST_STORE (filter->_attr_filter_store),
                        &iter,
                        ATTR_LOOK_IMAGE_str,  look_image,
                        ATTR_LOOK_VALUE_uint, look_value, -1);
  }

  g_free (look_image);

  filter->UpdateAttrList ();
}

// --------------------------------------------------------------------------------
void Filter::OnAttrDeleted (GtkTreeModel *tree_model,
                            GtkTreePath  *path,
                            Filter       *filter)
{
  filter->UpdateAttrList ();
}
