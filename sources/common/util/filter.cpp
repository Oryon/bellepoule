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

#include "attribute.hpp"
#include "module.hpp"
#include "global.hpp"
#include "user_config.hpp"
#include "xml_scheme.hpp"

#include "filter.hpp"

static const gchar *look_images[AttributeDesc::NB_LOOK] =
{
  N_ ("Long text"),
  N_ ("Short text"),
  N_ ("Graphic")
};

gboolean Filter::_no_persistence = FALSE;

// --------------------------------------------------------------------------------
Filter::Filter (const gchar *name,
                GSList      *attr_list)
: Object ("Filter")
{
  _dialog        = nullptr;
  _selected_list = nullptr;
  _attr_list     = attr_list;
  _owners        = nullptr;
  _name          = g_strdup (name);

  {
    GtkTreeIter iter;

    _look_store = gtk_list_store_new ((gint) StoreLookColumn::NUM_LOOK_COLS,
                                      G_TYPE_STRING, // IMAGE_str
                                      G_TYPE_UINT);  // VALUE_uint

    for (guint i = 0; i < AttributeDesc::NB_LOOK; i++)
    {
      gtk_list_store_append (_look_store, &iter);
      gtk_list_store_set (_look_store, &iter,
                          StoreLookColumn::IMAGE_str,  gettext (look_images[i]),
                          StoreLookColumn::VALUE_uint, i,
                          -1);
    }
  }

  {
    GSList *current = attr_list;

    _attr_filter_store = gtk_list_store_new ((gint) StoreAttrColumn::NUM_ATTR_COLS,
                                             G_TYPE_BOOLEAN, // VISIBILITY_bool
                                             G_TYPE_STRING,  // USER_NAME_str
                                             G_TYPE_POINTER, // XML_NAME_ptr
                                             G_TYPE_STRING,  // LOOK_IMAGE_str
                                             G_TYPE_UINT);   // LOOK_VALUE_uint

    while (current)
    {
      GtkTreeIter    iter;
      AttributeDesc *desc = (AttributeDesc *) current->data;

      gtk_list_store_append (_attr_filter_store, &iter);
      gtk_list_store_set (_attr_filter_store, &iter,
                          StoreAttrColumn::USER_NAME_str,   desc->_user_name,
                          StoreAttrColumn::XML_NAME_ptr,    desc->_code_name,
                          StoreAttrColumn::VISIBILITY_bool, FALSE,
                          StoreAttrColumn::LOOK_IMAGE_str,  gettext (look_images[desc->_favorite_look]),
                          StoreAttrColumn::LOOK_VALUE_uint, desc->_favorite_look,
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

  FreeFullGList(Layout, _selected_list);

  g_free         (_name);
  g_list_free    (_owners);
  g_slist_free   (_attr_list);
  g_object_unref (_attr_filter_store);
  g_object_unref (_look_store);
}

// --------------------------------------------------------------------------------
void Filter::PreventPersistence ()
{
  _no_persistence = TRUE;
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
void Filter::AddOwner (Module *owner)
{
  if (g_strstr_len (_name,
                    -1,
                    owner->GetKlassName ()) == nullptr)
  {
    gchar *name = g_strdup_printf ("%s::%s", _name, owner->GetKlassName ());

    g_free (_name);
    _name = name;
  }

  _owners = g_list_prepend (_owners,
                            owner);
}

// --------------------------------------------------------------------------------
void Filter::RemoveOwner (Module *owner)
{
  _owners = g_list_remove (_owners,
                           owner);
}

// --------------------------------------------------------------------------------
guint Filter::GetAttributeId (const gchar *name)
{
  GSList *current = _attr_list;

  for (guint i = 0; current != nullptr; i++)
  {
    AttributeDesc *desc = (AttributeDesc *) current->data;

    if (g_strcmp0 (desc->_code_name, name) == 0)
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
GList *Filter::GetLayoutList ()
{
  return _selected_list;
}

// --------------------------------------------------------------------------------
void Filter::ShowAttribute (const gchar *name)
{
  GtkTreeIter  iter;
  GtkTreeIter *sibling = nullptr;
  gboolean     iter_is_valid;

  iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_attr_filter_store),
                                                 &iter);

  while (iter_is_valid)
  {
    gboolean  current_visibility;
    gchar    *current_name;

    gtk_tree_model_get (GTK_TREE_MODEL (_attr_filter_store),
                        &iter,
                        StoreAttrColumn::VISIBILITY_bool, &current_visibility,
                        StoreAttrColumn::XML_NAME_ptr,    &current_name,
                        -1);

    if (current_visibility == TRUE)
    {
      if (sibling)
      {
        gtk_tree_iter_free (sibling);
      }
      sibling = gtk_tree_iter_copy (&iter);
    }

    if (g_strcmp0 (current_name, name) == 0)
    {
      Layout *attr_layout = new Layout ();

      gtk_list_store_set (GTK_LIST_STORE (_attr_filter_store),
                          &iter,
                          StoreAttrColumn::VISIBILITY_bool, TRUE, -1);
      gtk_list_store_move_before (GTK_LIST_STORE (_attr_filter_store),
                                  &iter,
                                  sibling);

      attr_layout->_desc = AttributeDesc::GetDescFromCodeName (current_name);
      attr_layout->_look = attr_layout->_desc->_favorite_look;
      _selected_list = g_list_append (_selected_list,
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
void Filter::RestoreLast ()
{
  if (_no_persistence == FALSE)
  {
    gchar **last_selection;

    last_selection = g_key_file_get_string_list (Global::_user_config->_key_file,
                                                 _name,
                                                 "display",
                                                 nullptr,
                                                 nullptr);

    if (last_selection)
    {
      ApplyList (last_selection);
      UpdateAttrList ();

      g_strfreev (last_selection);
    }
  }
}

// --------------------------------------------------------------------------------
void Filter::ApplyList (gchar **list)
{
  {
    GtkTreeIter iter;
    gboolean    iter_is_valid;

    iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_attr_filter_store),
                                                   &iter);

    while (iter_is_valid)
    {
      gtk_list_store_set (GTK_LIST_STORE (_attr_filter_store),
                          &iter,
                          StoreAttrColumn::VISIBILITY_bool, FALSE, -1);

      iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_attr_filter_store),
                                                &iter);
    }
  }

  FreeFullGList(Layout, _selected_list);

  for (guint i = 0; list[i]; i++)
  {
    ShowAttribute (list[i]);
  }
}

// --------------------------------------------------------------------------------
void Filter::Save (XmlScheme   *xml_scheme,
                   const gchar *as)
{
  GList   *current = _selected_list;
  GString *string  = g_string_new ("");

  for (guint i = 0; current; i++)
  {
    Layout *attr_layout = (Layout *) current->data;

    if (i > 0)
    {
      g_string_append_c (string, ',');
    }
    g_string_append (string, attr_layout->_desc->_xml_name);

    current = g_list_next (current);
  }

  {
    gchar *full_name = g_strdup_printf ("Affichage%s",
                                        as);

    xml_scheme->WriteFormatAttribute (full_name,
                                      "%s", string->str);
    g_free (full_name);
  }

  g_string_free (string,
                 TRUE);
}

// --------------------------------------------------------------------------------
void Filter::Load (xmlNode     *xml_node,
                   const gchar *as)
{
  gchar *full_name = g_strdup_printf ("Affichage%s", as);
  gchar *attr      = (gchar *) xmlGetProp (xml_node, BAD_CAST full_name);

  if (attr)
  {
    gchar **selections = g_strsplit (attr,
                                     ",",
                                     -1);

    if (selections)
    {
      for (guint i = 0; selections[i]; i++)
      {
        AttributeDesc *desc = AttributeDesc::GetDescFromXmlName (selections[i]);

        g_free (selections[i]);
        selections[i] = nullptr;

        if (desc)
        {
          selections[i] = g_strdup (desc->_code_name);
        }
      }

      ApplyList (selections);
      UpdateAttrList (FALSE);

      g_strfreev (selections);
    }

    xmlFree (attr);
  }
}

// --------------------------------------------------------------------------------
void Filter::SelectAttributes ()
{
  if (_dialog == nullptr)
  {
    _dialog = gtk_dialog_new_with_buttons (gettext ("Data to display"),
                                           nullptr,
                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_STOCK_CLOSE,
                                           GTK_RESPONSE_NONE,
                                           NULL);

    g_object_set (_dialog,
                  "transient-for",       gtk_widget_get_parent_window (_dialog),
                  "destroy-with-parent", TRUE,
                  "modal",               TRUE,
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
                                                     "active", StoreAttrColumn::VISIBILITY_bool,
                                                     NULL);
      }

      // Name
      {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();

        gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
                                                     -1,
                                                     gettext ("Data"),
                                                     renderer,
                                                     "text", StoreAttrColumn::USER_NAME_str,
                                                     NULL);
      }

      // Look
      {
        GtkCellRenderer *renderer = gtk_cell_renderer_combo_new ();

        g_object_set (G_OBJECT (renderer),
                      "model",       _look_store,
                      "text-column", StoreLookColumn::IMAGE_str,
                      "editable",    TRUE,
                      "has-entry",   FALSE,
                      NULL);
        g_signal_connect (renderer,
                          "changed", G_CALLBACK (OnLookChanged), this);
        gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
                                                     -1,
                                                     gettext ("Look"),
                                                     renderer,
                                                     "text", StoreAttrColumn::LOOK_IMAGE_str,
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
void Filter::UpdateAttrList (gboolean save_it)
{
  FreeFullGList(Layout, _selected_list);

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
                          StoreAttrColumn::VISIBILITY_bool, &current_visibility,
                          StoreAttrColumn::XML_NAME_ptr,    &current_name,
                          StoreAttrColumn::LOOK_VALUE_uint, &curent_look,
                          -1);

      if (current_visibility == TRUE)
      {
        Layout *attr_layout = new Layout ();

        attr_layout->_look = curent_look;
        attr_layout->_desc = AttributeDesc::GetDescFromCodeName (current_name);
        _selected_list = g_list_append (_selected_list,
                                        attr_layout);
      }

      iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_attr_filter_store),
                                                &iter);
    }
  }

  // Make the choice persistent
  if (save_it && (_no_persistence == FALSE))
  {
    GList  *current    = _selected_list;
    gsize   length     = g_list_length (current);
    gchar **value_list = g_new (gchar *, length);

    for (guint i = 0; current; i++)
    {
      Layout *attr_layout = (Layout *) current->data;

      value_list[i] = attr_layout->_desc->_code_name;

      current = g_list_next (current);
    }

    g_key_file_set_string_list (Global::_user_config->_key_file,
                                _name,
                                "display",
                                value_list,
                                length);
    g_free (value_list);
  }

  {
    GList *current = _owners;

    while (current)
    {
      Module *owner = (Module *) current->data;

      if (owner->IsPlugged ())
      {
        owner->OnAttrListUpdated ();
        owner->MakeDirty ();
      }
      current = g_list_next (current);
    }
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
                        StoreAttrColumn::VISIBILITY_bool, visibility,
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
                      StoreLookColumn::IMAGE_str,  &look_image,
                      StoreLookColumn::VALUE_uint, &look_value,
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
                        StoreAttrColumn::LOOK_IMAGE_str,  look_image,
                        StoreAttrColumn::LOOK_VALUE_uint, look_value, -1);
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
