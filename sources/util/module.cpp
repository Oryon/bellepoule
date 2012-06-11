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

GtkTargetEntry Module::_dnd_target_list[] =
{
  {"REFEREE", GTK_TARGET_SAME_APP|GTK_TARGET_OTHER_WIDGET, Module::INT_TARGET}
};

// --------------------------------------------------------------------------------
Module::Module (const gchar *glade_file,
                const gchar *root)
//: Object ("Module")
{
  _plugged_list   = NULL;
  _owner          = NULL;
  _data_owner     = this;
  _root           = NULL;
  _glade          = NULL;
  _toolbar        = NULL;
  _filter         = NULL;
  _rand_seed      = 0;

  _print_settings            = gtk_print_settings_new ();
  _page_setup_print_settings = gtk_print_settings_new ();
  _default_page_setup        = gtk_page_setup_new     ();

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

  Object::TryToRelease (_glade);

  if (_root)
  {
    g_object_unref (_root);
  }

  if (_config_widget)
  {
    g_object_unref (_config_widget);
  }

  Object::TryToRelease (_filter);

  g_object_unref (_print_settings);
  g_object_unref (_page_setup_print_settings);
  g_object_unref (_default_page_setup);
}

// --------------------------------------------------------------------------------
void Module::DragDataGet (GtkWidget        *widget,
                          GdkDragContext   *drag_context,
                          GtkSelectionData *data,
                          guint             info,
                          guint             time,
                          Module           *owner)
{
  owner->OnDragDataGet (widget,
                        drag_context,
                        data,
                        info,
                        time);
}

// --------------------------------------------------------------------------------
void Module::OnDragDataGet (GtkWidget        *widget,
                            GdkDragContext   *drag_context,
                            GtkSelectionData *data,
                            guint             info,
                            guint             time)
{
}

// --------------------------------------------------------------------------------
gboolean Module::DragDrop (GtkWidget      *widget,
                           GdkDragContext *drag_context,
                           gint            x,
                           gint            y,
                           guint           time,
                           Module         *owner)
{
  return owner->OnDragDrop (widget,
                            drag_context,
                            x,
                            y,
                            time);
}

// --------------------------------------------------------------------------------
gboolean Module::OnDragDrop (GtkWidget      *widget,
                             GdkDragContext *drag_context,
                             gint            x,
                             gint            y,
                             guint           time)
{
  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean Module::DragMotion (GtkWidget      *widget,
                             GdkDragContext *drag_context,
                             gint            x,
                             gint            y,
                             guint           time,
                             Module         *owner)
{
  return owner->OnDragMotion (widget,
                              drag_context,
                              x,
                              y,
                              time);
}

// --------------------------------------------------------------------------------
gboolean Module::OnDragMotion (GtkWidget      *widget,
                               GdkDragContext *drag_context,
                               gint            x,
                               gint            y,
                               guint           time)
{
  gdk_drag_status  (drag_context,
                    (GdkDragAction) 0,
                    time);
  return FALSE;
}

// --------------------------------------------------------------------------------
void Module::DragLeave (GtkWidget      *widget,
                        GdkDragContext *drag_context,
                        guint           time,
                        Module         *owner)
{
  owner->OnDragLeave (widget,
                      drag_context,
                      time);
}

// --------------------------------------------------------------------------------
void Module::OnDragLeave (GtkWidget      *widget,
                          GdkDragContext *drag_context,
                          guint           time)
{
}

// --------------------------------------------------------------------------------
void Module::DragDataReceived (GtkWidget        *widget,
                               GdkDragContext   *drag_context,
                               gint              x,
                               gint              y,
                               GtkSelectionData *data,
                               guint             info,
                               guint             time,
                               Module           *owner)
{
  owner->OnDragDataReceived (widget,
                             drag_context,
                             x,
                             y,
                             data,
                             info,
                             time);
}

// --------------------------------------------------------------------------------
void Module::OnDragDataReceived (GtkWidget        *widget,
                                 GdkDragContext   *drag_context,
                                 gint              x,
                                 gint              y,
                                 GtkSelectionData *data,
                                 guint             info,
                                 guint             time)
{
}

// --------------------------------------------------------------------------------
void Module::SetDndSource (GtkWidget *widget)
{
  gtk_drag_source_set (widget,
                       GDK_MODIFIER_MASK,
                       _dnd_target_list,
                       sizeof (_dnd_target_list) / sizeof (_dnd_target_list[0]),
                       GDK_ACTION_COPY);

  gtk_drag_source_add_text_targets (widget);

  g_signal_connect (widget, "drag-data-get",
                    G_CALLBACK (DragDataGet), this);
}

// --------------------------------------------------------------------------------
void Module::SetDndDest (GtkWidget *widget)
{
  gtk_drag_dest_set (widget,
                     (GtkDestDefaults) 0,
                     _dnd_target_list,
                     sizeof (_dnd_target_list) / sizeof (_dnd_target_list[0]),
                     GDK_ACTION_COPY);

  gtk_drag_dest_add_text_targets (widget);

  g_signal_connect (widget, "drag-motion",
                    G_CALLBACK (DragMotion), this);
  g_signal_connect (widget, "drag-leave",
                    G_CALLBACK (DragLeave), this);
  g_signal_connect (widget, "drag-drop",
                    G_CALLBACK (DragDrop), this);
  g_signal_connect (widget, "drag-data-received",
                    G_CALLBACK (DragDataReceived), this);
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
  if (module)
  {
    if (in)
    {
      gtk_container_add (GTK_CONTAINER (in),
                         module->_root);
    }

    module->_toolbar = toolbar;

    _plugged_list = g_slist_append (_plugged_list,
                                    module);
    module->_owner = this;

    module->OnPlugged ();
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

    OnUnPlugged ();

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
  }
}

// --------------------------------------------------------------------------------
void Module::RefreshMatchRate (gint delta)
{
  if (_owner)
  {
    _owner->RefreshMatchRate (delta);
  }
}

// --------------------------------------------------------------------------------
void Module::RefreshMatchRate (Player *player)
{
  if (_owner)
  {
    _owner->RefreshMatchRate (player);
  }
}

// --------------------------------------------------------------------------------
Module::State Module::GetState ()
{
  if (_owner)
  {
    return _owner->GetState ();
  }
  else
  {
    return OPERATIONAL;
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
  _sensitivity_trigger.AddWidget (w);
}

// --------------------------------------------------------------------------------
void Module::EnableSensitiveWidgets ()
{
  _sensitivity_trigger.SwitchOn ();
}

// --------------------------------------------------------------------------------
void Module::DisableSensitiveWidgets ()
{
  _sensitivity_trigger.SwitchOff ();
}

// --------------------------------------------------------------------------------
void Module::SetCursor (GdkCursorType cursor_type)
{
  GdkCursor *cursor = gdk_cursor_new (cursor_type);

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
    GSList *selected_attr = NULL;

    if (_filter)
    {
      selected_attr = _filter->GetSelectedAttrList ();
    }

    for (guint a = 0; selected_attr != NULL; a++)
    {
      AttributeDesc       *attr_desc;
      Attribute           *attr;
      Player::AttributeId *attr_id;

      attr_desc = (AttributeDesc *) selected_attr->data;
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

      selected_attr = g_slist_next (selected_attr);
    }

    image = g_string_append (image,
                             " \302\240\302\240");
  }

  return image;
}

// --------------------------------------------------------------------------------
void Module::Print (const gchar  *job_name,
                    GtkPageSetup *page_setup)
{
  Print (job_name,
         NULL,
         page_setup,
         GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG);
}

// --------------------------------------------------------------------------------
void Module::PrintPDF (const gchar  *job_name,
                       const gchar  *filename)
{
  Print (job_name,
         filename,
         NULL,
         GTK_PRINT_OPERATION_ACTION_EXPORT);
}

// --------------------------------------------------------------------------------
void Module::PrintPreview (const gchar  *job_name,
                           GtkPageSetup *page_setup)
{
  Print (job_name,
         NULL,
         page_setup,
         GTK_PRINT_OPERATION_ACTION_PREVIEW);
}

// --------------------------------------------------------------------------------
void Module::Print (const gchar             *job_name,
                    const gchar             *filename,
                    GtkPageSetup            *page_setup,
                    GtkPrintOperationAction  action)
{
  GtkPrintOperationResult  res;
  GError                  *error     = NULL;
  GtkPrintOperation       *operation = gtk_print_operation_new ();

  if (job_name)
  {
    g_object_set_data (G_OBJECT (operation), "job_name", (void *) job_name);
  }

  {
    gchar *full_name = g_strdup_printf ("BellePoule - %s", job_name);

    gtk_print_operation_set_job_name (operation,
                                      full_name);
    g_free (full_name);
  }

  if (page_setup)
  {
    gtk_print_operation_set_default_page_setup (operation,
                                                page_setup);
    gtk_print_operation_set_print_settings (operation,
                                            _page_setup_print_settings);
  }
  else
  {
    gtk_print_operation_set_default_page_setup (operation,
                                                _default_page_setup);
    gtk_print_operation_set_print_settings (operation,
                                            _print_settings);
  }

  g_signal_connect (G_OBJECT (operation), "begin-print",
                    G_CALLBACK (on_begin_print), this);
  g_signal_connect (G_OBJECT (operation), "draw-page",
                    G_CALLBACK (on_draw_page), this);
  g_signal_connect (G_OBJECT (operation), "end-print",
                    G_CALLBACK (on_end_print), this);
  g_signal_connect (G_OBJECT (operation), "ready",
                    G_CALLBACK (on_preview_ready), this);
  g_signal_connect (G_OBJECT (operation), "preview",
                    G_CALLBACK (on_preview), this);
  g_signal_connect (G_OBJECT (operation), "got-page-size",
                    G_CALLBACK (on_preview_got_page_size), this);

  if (filename)
  {
    gtk_print_operation_set_export_filename (operation,
                                             filename);
  }

  res = gtk_print_operation_run (operation,
                                 action,
                                 NULL,
                                 &error);

  if (res == GTK_PRINT_OPERATION_RESULT_APPLY)
  {
    GtkPrintSettings *operation_print_settings = gtk_print_operation_get_print_settings (operation);

    if (operation_print_settings)
    {
      if (page_setup)
      {
        g_object_unref (_page_setup_print_settings);

        _page_setup_print_settings = operation_print_settings;
        g_object_ref (_page_setup_print_settings);
      }
      else
      {
        g_object_unref (_print_settings);

        _print_settings = operation_print_settings;
        g_object_ref (_print_settings);
      }
    }
  }

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
void Module::MakeDirty ()
{
  if (_owner)
  {
    _owner->MakeDirty ();
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
void Module::on_preview_ready (GtkPrintOperationPreview *preview,
                               GtkPrintContext          *context,
                               Module                   *module)
{
  module->OnPreviewReady (preview,
                          context);
}

// --------------------------------------------------------------------------------
void Module::on_preview_got_page_size (GtkPrintOperationPreview *preview,
                                       GtkPrintContext          *context,
                                       GtkPageSetup             *page_setup,
                                       Module                   *module)
{
  module->OnPreviewGotPageSize (preview,
                                context,
                                page_setup);
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
    AttributeDesc *desc = AttributeDesc::GetDescFromCodeName ("status");

    if (desc)
    {
      GtkTreeIter  iter;
      gboolean     iter_is_valid;

      _status_model = GTK_TREE_MODEL (gtk_tree_store_new (4, G_TYPE_UINT,
                                                          G_TYPE_STRING,
                                                          G_TYPE_STRING,
                                                          GDK_TYPE_PIXBUF));

      iter_is_valid = gtk_tree_model_get_iter_first (desc->_discrete_model,
                                                     &iter);
      for (guint i = 0; iter_is_valid; i++)
      {
        gchar     *xml_image;
        gchar     *user_image;
        GdkPixbuf *icon;

        gtk_tree_model_get (desc->_discrete_model,
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
        iter_is_valid = gtk_tree_model_iter_next (desc->_discrete_model,
                                                  &iter);
      }
    }
  }

  return _status_model;
}
