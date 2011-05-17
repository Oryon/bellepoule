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
#include "player.hpp"

#include "module.hpp"

const gdouble Module::PRINT_HEADER_HEIGHT = 10.0; // % of paper width
const gdouble Module::PRINT_FONT_HEIGHT   = 2.0;  // % of paper width
GKeyFile     *Module::_config_file  = NULL;
GtkTreeModel *Module::_status_model = NULL;

// --------------------------------------------------------------------------------
Module::Module (const gchar *glade_file,
                const gchar *root)
//: Object ("Module")
{
  _plugged_list = NULL;
  _owner        = NULL;
  _data_owner   = this;
  _root         = NULL;
  _glade        = NULL;
  _toolbar      = NULL;
  _filter       = NULL;
  _rand_seed    = 0;

  _sensitivity_trigger = new SensitivityTrigger ();

  if (glade_file)
  {
    _glade = new Glade (glade_file,
                        this);

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
    _config_widget = _glade->GetWidget ("stage_configuration");

    if (_config_widget)
    {
      g_object_ref (_config_widget);
      gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (_config_widget)),
                                           _config_widget);
    }
  }
}

// --------------------------------------------------------------------------------
Module::~Module ()
{
  while (_plugged_list)
  {
    Module *module;

    module = (Module *) (g_slist_nth_data (_plugged_list, 0));
    module->UnPlug ();
  }

  UnPlug ();

  _sensitivity_trigger->Release ();

  Object::TryToRelease (_glade);
  g_object_unref (_root);

  if (_config_widget)
  {
    g_object_unref (_config_widget);
  }

  Object::TryToRelease (_filter);
}

// --------------------------------------------------------------------------------
void Module::SetDataOwner (Object *data_owner)
{
  _data_owner = data_owner;
}

// --------------------------------------------------------------------------------
Object *Module::GetDataOwner ()
{
  return _data_owner;
}

// --------------------------------------------------------------------------------
GtkWidget *Module::GetConfigWidget ()
{
  return _config_widget;
}

// --------------------------------------------------------------------------------
GtkWidget *Module::GetWidget (const gchar *name)
{
  return _glade->GetWidget (name);
}

// --------------------------------------------------------------------------------
void Module::SetFilter (Filter *filter)
{
  _filter = filter;

  if (_filter)
  {
    _filter->Retain ();
  }
}

// --------------------------------------------------------------------------------
void Module::SelectAttributes ()
{
  _filter->SelectAttributes ();
}

// --------------------------------------------------------------------------------
Filter *Module::GetFilter ()
{
  if (_filter)
  {
    _filter->Retain ();
  }
  return _filter;
}

// --------------------------------------------------------------------------------
void Module::Plug (Module     *module,
                   GtkWidget  *in,
                   GtkToolbar *toolbar)
{
  if (module && in)
  {
    gtk_container_add (GTK_CONTAINER (in),
                       module->_root);

    module->_toolbar = toolbar;

    module->OnPlugged ();

    _plugged_list = g_slist_append (_plugged_list,
                                    module);
    module->_owner = this;
  }
}

// --------------------------------------------------------------------------------
void Module::UnPlug ()
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

    if (_filter)
    {
      _filter->UnPlug ();
    }

    OnUnPlugged ();
  }
}

// --------------------------------------------------------------------------------
gboolean Module::IsPlugged ()
{
  return (gtk_widget_get_parent (_root) != NULL);
}

// --------------------------------------------------------------------------------
GtkWidget *Module::GetRootWidget ()
{
  return _root;
}

// --------------------------------------------------------------------------------
GtkToolbar *Module::GetToolbar ()
{
  return _toolbar;
}

// --------------------------------------------------------------------------------
void Module::AddSensitiveWidget (GtkWidget *w)
{
  _sensitivity_trigger->AddWidget (w);
}

// --------------------------------------------------------------------------------
void Module::EnableSensitiveWidgets ()
{
  _sensitivity_trigger->SwitchOn ();
}

// --------------------------------------------------------------------------------
void Module::DisableSensitiveWidgets ()
{
  _sensitivity_trigger->SwitchOff ();
}

// --------------------------------------------------------------------------------
void Module::SetCursor (GdkCursorType cursor_type)
{
  GdkCursor *cursor;

  cursor = gdk_cursor_new (cursor_type);
  gdk_window_set_cursor (gtk_widget_get_window (_root),
                         cursor);
  gdk_cursor_unref (cursor);
}

// --------------------------------------------------------------------------------
void Module::ResetCursor ()
{
  gdk_window_set_cursor (gtk_widget_get_window (_root),
                         NULL);
}

// --------------------------------------------------------------------------------
GString *Module::GetPlayerImage (Player *player)
{
  GString *image = g_string_new ("");

  if (player)
  {
    GSList  *selected_attr = NULL;

    if (_filter)
    {
      selected_attr = _filter->GetSelectedAttrList ();
    }

    for (guint a = 0; a < g_slist_length (selected_attr); a++)
    {
      AttributeDesc       *attr_desc;
      Attribute           *attr;
      Player::AttributeId *attr_id;

      attr_desc = (AttributeDesc *) g_slist_nth_data (selected_attr,
                                                      a);
      if (attr_desc->_scope == AttributeDesc::LOCAL)
      {
        attr_id = new Player::AttributeId (attr_desc->_code_name,
                                           GetDataOwner ());
      }
      else
      {
        attr_id = new Player::AttributeId (attr_desc->_code_name);
      }
      attr = player->GetAttribute (attr_id);
      attr_id->Release ();

      if (attr)
      {
        gchar *attr_image = attr->GetUserImage ();

        if (a > 0)
        {
          image = g_string_append (image,
                                   " - ");
        }
        image = g_string_append (image,
                                 attr_image);
        g_free (attr_image);
      }
    }

    image = g_string_append (image,
                             " \302\240\302\240");
  }

  return image;
}

// --------------------------------------------------------------------------------
void Module::Print (const gchar *job_name)
{
  GtkPrintOperation *operation;
  GError            *error = NULL;

  operation = gtk_print_operation_new ();

  g_object_set_data (G_OBJECT (operation), "job_name", (void *) job_name);

  {
    gchar *full_name = g_strdup_printf ("BellePoule - %s", job_name);

    gtk_print_operation_set_job_name (operation,
                                      full_name);
    g_free (full_name);
  }

  g_signal_connect (G_OBJECT (operation), "begin-print",
                    G_CALLBACK (on_begin_print), this);
  g_signal_connect (G_OBJECT (operation), "draw-page",
                    G_CALLBACK (on_draw_page), this);
  g_signal_connect (G_OBJECT (operation), "end-print",
                    G_CALLBACK (on_end_print), this);
  g_signal_connect (G_OBJECT (operation), "preview",
                    G_CALLBACK (on_preview), this);

  //gtk_print_operation_set_use_full_page (operation,
  //TRUE);

  //gtk_print_operation_set_unit (operation,
  //GTK_UNIT_POINTS);

  gtk_print_operation_run (operation,
                           GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
                           //GTK_PRINT_OPERATION_ACTION_PREVIEW,
                           NULL,
                           &error);

  g_object_unref (operation);

  if (error)
  {
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new (NULL,
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     "%s", error->message);
    g_error_free (error);

    g_signal_connect (dialog, "response",
                      G_CALLBACK (gtk_widget_destroy), NULL);

    gtk_widget_show (dialog);
  }
}

// --------------------------------------------------------------------------------
void Module::OnDrawPage (GtkPrintOperation *operation,
                         GtkPrintContext   *context,
                         gint               page_nr)
{
  DrawContainerPage (operation,
                     context,
                     page_nr);
}

// --------------------------------------------------------------------------------
void Module::DrawContainerPage (GtkPrintOperation *operation,
                                GtkPrintContext   *context,
                                gint               page_nr)
{
  if (_owner)
  {
    _owner->OnDrawPage (operation,
                        context,
                        page_nr);
  }
}

// --------------------------------------------------------------------------------
void Module::on_begin_print (GtkPrintOperation *operation,
                             GtkPrintContext   *context,
                             Module            *module)
{
  module->OnBeginPrint (operation,
                        context);
}

// --------------------------------------------------------------------------------
void Module::on_draw_page (GtkPrintOperation *operation,
                           GtkPrintContext   *context,
                           gint               page_nr,
                           Module            *module)
{
  module->OnDrawPage (operation,
                      context,
                      page_nr);
}

// --------------------------------------------------------------------------------
gboolean Module::on_preview (GtkPrintOperation        *operation,
                             GtkPrintOperationPreview *preview,
                             GtkPrintContext          *context,
                             GtkWindow                *parent,
                             Module                   *module)
{
  return module->OnPreview (operation,
                            preview,
                            context,
                            parent);
}

// --------------------------------------------------------------------------------
void Module::on_end_print (GtkPrintOperation *operation,
                           GtkPrintContext   *context,
                           Module            *module)
{
  module->OnEndPrint (operation,
                      context);
}

// --------------------------------------------------------------------------------
GtkTreeModel *Module::GetStatusModel ()
{
  if (_status_model == NULL)
  {
    AttributeDesc *desc = AttributeDesc::GetDesc ("status");

    if (desc)
    {
      GtkTreeIter  iter;
      gboolean     iter_is_valid;

      _status_model = GTK_TREE_MODEL (gtk_tree_store_new (4, G_TYPE_UINT,
                                                          G_TYPE_STRING,
                                                          G_TYPE_STRING,
                                                          GDK_TYPE_PIXBUF));

      iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (desc->_discrete_store),
                                                     &iter);
      for (guint i = 0; iter_is_valid; i++)
      {
        gchar     *xml_image;
        gchar     *user_image;
        GdkPixbuf *icon;

        gtk_tree_model_get (GTK_TREE_MODEL (desc->_discrete_store),
                            &iter,
                            AttributeDesc::DISCRETE_XML_IMAGE, &xml_image,
                            AttributeDesc::DISCRETE_USER_IMAGE, &user_image,
                            AttributeDesc::DISCRETE_ICON, &icon, -1);
        if (   (strcmp ("Q", xml_image) == 0)
            || (strcmp ("A", xml_image) == 0)
            || (strcmp ("E", xml_image) == 0))
        {
          GtkTreeIter pool_iter;

          gtk_tree_store_append (GTK_TREE_STORE (_status_model), &pool_iter, NULL);
          gtk_tree_store_set (GTK_TREE_STORE (_status_model), &pool_iter,
                              AttributeDesc::DISCRETE_XML_IMAGE, xml_image,
                              AttributeDesc::DISCRETE_USER_IMAGE, user_image,
                              AttributeDesc::DISCRETE_ICON, icon,
                              -1);
        }
        iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (desc->_discrete_store),
                                                  &iter);
      }
    }
  }

  return _status_model;
}
