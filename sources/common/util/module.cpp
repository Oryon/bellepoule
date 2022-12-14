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
#include "glade.hpp"
#include "dnd_config.hpp"
#include "filter.hpp"
#include "canvas.hpp"

#include "module.hpp"

const gdouble  Module::PRINT_HEADER_FRAME_HEIGHT = 10.0; // % of paper width
const gdouble  Module::PRINT_FONT_HEIGHT         = 2.0;  // % of paper width
GtkTreeModel  *Module::_status_model             = nullptr;
GtkWindow     *Module::_main_window              = nullptr;

// --------------------------------------------------------------------------------
Module::Module (const gchar *glade_file,
                const gchar *root)
{
   _plugged_list     = nullptr;
   _owner            = nullptr;
   _data_owner       = this;
   _root             = nullptr;
   _glade            = nullptr;
   _toolbar          = nullptr;
   _config_container = nullptr;
   _filter           = nullptr;

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

    if (_root)
    {
      gtk_widget_set_name (_root,
                           glade_file);

      if (_main_window == nullptr)
      {
        _main_window = GTK_WINDOW (_root);
      }
    }

    _glade->DetachFromParent (_root);
  }

  {
    _config_widget = _glade->GetWidget ("stage_configuration");

    _glade->DetachFromParent (_config_widget);
  }

  _dnd_config = new DndConfig ();
}

// --------------------------------------------------------------------------------
Module::~Module ()
{
  if (_filter)
  {
    _filter->RemoveOwner (this);
  }

  _dnd_config->Release ();

  while (_plugged_list)
  {
    Module *module;

    module = (Module *) (g_slist_nth_data (_plugged_list, 0));
    module->UnPlug ();
  }

  UnPlug ();

  Object::TryToRelease (_filter);

  g_object_unref (_print_settings);
  g_object_unref (_page_setup_print_settings);
  g_object_unref (_default_page_setup);

  Object::TryToRelease (_glade);
}

// --------------------------------------------------------------------------------
DndConfig *Module::GetDndConfig ()
{
  return _dnd_config;
}

// --------------------------------------------------------------------------------
void Module::DragDataGet (GtkWidget        *widget,
                          GdkDragContext   *drag_context,
                          GtkSelectionData *data,
                          guint             key,
                          guint             time,
                          Module           *owner)
{
  owner->OnDragDataGet (widget,
                        drag_context,
                        data,
                        key,
                        time);
}

// --------------------------------------------------------------------------------
void Module::OnDragDataGet (GtkWidget        *widget,
                            GdkDragContext   *drag_context,
                            GtkSelectionData *data,
                            guint             key,
                            guint             time)
{
}

// --------------------------------------------------------------------------------
void Module::DragEnd (GtkWidget      *widget,
                      GdkDragContext *drag_context,
                      Module         *owner)
{
  owner->OnDragEnd (widget,
                    drag_context);
}

// --------------------------------------------------------------------------------
void Module::OnDragEnd (GtkWidget      *widget,
                        GdkDragContext *drag_context)
{
  _dnd_config->DragEnd ();
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
  GdkAtom atom = gtk_drag_dest_find_target (widget,
                                            drag_context,
                                            nullptr);

  return (atom != nullptr);
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
  gtk_drag_highlight (widget);

  {
    GdkAtom atom = gtk_drag_dest_find_target (widget,
                                              drag_context,
                                              nullptr);

    if (atom)
    {
      _dnd_config->SetContext (drag_context);

      if (_dnd_config->GetFloatingObject () == nullptr)
      {
        gtk_drag_get_data (widget,
                           drag_context,
                           atom,
                           time);
      }

      gdk_drag_status (drag_context,
                       gdk_drag_context_get_suggested_action (drag_context),
                       time);
      return TRUE;
    }
  }

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
                               guint             key,
                               guint             time,
                               Module           *owner)
{
  owner->OnDragDataReceived (widget,
                             drag_context,
                             x,
                             y,
                             data,
                             key,
                             time);
}

// --------------------------------------------------------------------------------
void Module::OnDragDataReceived (GtkWidget        *widget,
                                 GdkDragContext   *drag_context,
                                 gint              x,
                                 gint              y,
                                 GtkSelectionData *data,
                                 guint             key,
                                 guint             time)
{
  if (data && (gtk_selection_data_get_length (data) >= 0))
  {
    guint32 *ref = (guint32 *) gtk_selection_data_get_data (data);

    _dnd_config->SetFloatingObject (GetDropObjectFromRef (*ref,
                                                          key));
  }
}

// --------------------------------------------------------------------------------
Object *Module::GetDropObjectFromRef (guint32 ref,
                                      guint   key)
{
  return nullptr;
}

// --------------------------------------------------------------------------------
void Module::ConnectDndSource (GtkWidget *widget)
{
  g_signal_connect (widget, "drag-data-get",
                    G_CALLBACK (DragDataGet), this);
  g_signal_connect (widget, "drag-end",
                    G_CALLBACK (DragEnd), this);
}

// --------------------------------------------------------------------------------
void Module::ConnectDndDest (GtkWidget *widget)
{
  g_signal_connect (widget, "drag-motion",
                    G_CALLBACK (DragMotion), this);
  g_signal_connect (widget, "drag-drop",
                    G_CALLBACK (DragDrop), this);
  g_signal_connect (widget, "drag-data-received",
                    G_CALLBACK (DragDataReceived), this);
  g_signal_connect (widget, "drag-leave",
                    G_CALLBACK (DragLeave), this);
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
GObject *Module::GetGObject (const gchar *name)
{
  return _glade->GetGObject (name);
}

// --------------------------------------------------------------------------------
void Module::SetFilter (Filter *filter)
{
  if (_filter)
  {
    _filter->Release ();
  }

  _filter = filter;

  if (_filter)
  {
    _filter->Retain ();
    _filter->AddOwner (this);
    _filter->RestoreLast ();
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
void Module::Plug (Module       *module,
                   GtkWidget    *in,
                   GtkToolbar   *toolbar,
                   GtkContainer *config_container)
{
  if (module)
  {
    if (in)
    {
      gtk_container_add (GTK_CONTAINER (in),
                         module->_root);
    }

    module->_toolbar          = toolbar;
    module->_config_container = config_container;

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
      _owner = nullptr;
    }

    if (_filter)
    {
      _filter->UnPlug ();
    }
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
    return State::OPERATIONAL;
  }
}

// --------------------------------------------------------------------------------
gboolean Module::IsPlugged ()
{
  return (gtk_widget_get_parent (_root) != nullptr);
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
GtkContainer *Module::GetConfigContainer ()
{
  return _config_container;
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
                         nullptr);
}

// --------------------------------------------------------------------------------
void Module::Print (const gchar *job_name,
                    Object      *data)
{
  Print (job_name,
         nullptr,
         data,
         nullptr,
         GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG);
}

// --------------------------------------------------------------------------------
void Module::PrintPDF (const gchar  *job_name,
                       const gchar  *filename)
{
  Print (job_name,
         filename,
         nullptr,
         nullptr,
         GTK_PRINT_OPERATION_ACTION_EXPORT);
}

// --------------------------------------------------------------------------------
void Module::PrintPreview (const gchar *job_name,
                           Object      *data)
{
  Print (job_name,
         nullptr,
         data,
         nullptr,
         GTK_PRINT_OPERATION_ACTION_PREVIEW);
}

// --------------------------------------------------------------------------------
void Module::Print (const gchar             *job_name,
                    const gchar             *filename,
                    Object                  *data,
                    GtkPageSetup            *page_setup,
                    GtkPrintOperationAction  action)
{
  GtkPrintOperationResult  res;
  GError                  *error     = nullptr;
  GtkPrintOperation       *operation = gtk_print_operation_new ();

  if (data)
  {
    data->Retain ();
    g_object_set_data_full (G_OBJECT (operation),
                            "Print::Data", data,
                            (GDestroyNotify) Object::TryToRelease);
  }

  if (job_name)
  {
    g_object_set_data_full (G_OBJECT (operation),
                            "Print::PageName", (void *) g_strdup (job_name),
                            g_free);
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
                                 nullptr,
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

  g_object_unref (operation);

  if (error)
  {
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new (nullptr,
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
gdouble Module::GetPrintHeaderSize (GtkPrintContext *context,
                                    SizeReferential  referential)
{
  gdouble size = PRINT_HEADER_FRAME_HEIGHT + 2.0;

  if (referential == SizeReferential::ON_SHEET)
  {
    return GetSizeOnSheet (context,
                           size);
  }

  return size;
}

// --------------------------------------------------------------------------------
gdouble Module::GetPrintBodySize (GtkPrintContext *context,
                                  SizeReferential  referential)
{
  gdouble paper_w = gtk_print_context_get_width  (context);
  gdouble paper_h = gtk_print_context_get_height (context);
  gdouble size    = paper_h*100.0/paper_w - GetPrintHeaderSize (context, referential);

  if (referential == SizeReferential::ON_SHEET)
  {
    return GetSizeOnSheet (context,
                           size);
  }

  return size;
}

// --------------------------------------------------------------------------------
gdouble Module::GetSizeOnSheet (GtkPrintContext *context,
                                gdouble          normalized_size)
{
  gdouble paper_w = gtk_print_context_get_width  (context);

  return normalized_size * paper_w  / 100;
}

// --------------------------------------------------------------------------------
void Module::OnBeginPrint (GtkPrintOperation *operation,
                           GtkPrintContext   *context)
{
  guint n_pages = PreparePrint (operation,
                                context);

  gtk_print_operation_set_n_pages (operation,
                                   n_pages);
}


// --------------------------------------------------------------------------------
void Module::OnDrawPage (GtkPrintOperation *operation,
                         GtkPrintContext   *context,
                         gint               page_nr)
{
  DrawPage (operation,
            context,
            page_nr);
}

// --------------------------------------------------------------------------------
void Module::DrawPage (GtkPrintOperation *operation,
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
    _owner->DrawPage (operation,
                      context,
                      page_nr);
  }
}

// --------------------------------------------------------------------------------
void Module::DrawConfig (GtkPrintOperation *operation,
                         GtkPrintContext   *context,
                         gint               page_nr)
{
}

// --------------------------------------------------------------------------------
void Module::DrawConfigLine (GtkPrintOperation *operation,
                             GtkPrintContext   *context,
                             const gchar       *line)
{
  cairo_t *cr      = gtk_print_context_get_cairo_context (context);
  gdouble  paper_w = gtk_print_context_get_width (context);

  cairo_translate (cr,
                   0.0,
                   2.0 * paper_w / 100);

  {
    GooCanvas *canvas = Canvas::CreatePrinterCanvas (context);

    goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                         line,
                         1.0 * paper_w / 100,
                         0.0,
                         -1.0,
                         GTK_ANCHOR_W,
                         "font", BP_FONT "2px", NULL);

    goo_canvas_render (canvas,
                       gtk_print_context_get_cairo_context (context),
                       nullptr,
                       1.0);

    gtk_widget_destroy (GTK_WIDGET (canvas));
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
  return module->PreparePreview (operation,
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
void Module::DumpToHTML (FILE *file)
{
}

// --------------------------------------------------------------------------------
GdkPixbuf *Module::GetPixbuf (const gchar *icon)
{
  GdkPixbuf *pixbuf;

  pixbuf = gdk_pixbuf_new_from_file (icon, nullptr);
  if (pixbuf)
  {
    return pixbuf;
  }
  else
  {
    GtkWidget *image = gtk_image_new ();

    g_object_ref_sink (image);
    pixbuf = gtk_widget_render_icon (image,
                                     icon,
                                     GTK_ICON_SIZE_BUTTON,
                                     nullptr);
    g_object_unref (image);
  }

  return pixbuf;
}

// --------------------------------------------------------------------------------
gint Module::RunDialog (GtkDialog *dialog)
{
  gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                _main_window);
  return gtk_dialog_run (dialog);
}

// --------------------------------------------------------------------------------
void Module::Raise (GtkDialog *dialog,
                    GtkWindow *over)
{
  if (over == nullptr)
  {
    gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                  _main_window);
  }
  else
  {
    gtk_window_set_modal (GTK_WINDOW (dialog),
                          TRUE);
    gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                  over);
  }

  gtk_widget_show_all (GTK_WIDGET (dialog));
  gtk_widget_grab_focus (GTK_WIDGET (dialog));
}

// --------------------------------------------------------------------------------
GtkTreeModel *Module::GetStatusModel ()
{
  if (_status_model == nullptr)
  {
    AttributeDesc *desc = AttributeDesc::GetDescFromCodeName ("status");

    if (desc)
    {
      GtkTreeIter iter;
      gboolean    iter_is_valid;

      _status_model = GTK_TREE_MODEL (gtk_tree_store_new (5,
                                                          G_TYPE_UINT,
                                                          G_TYPE_STRING,
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
                            AttributeDesc::DiscreteColumnId::XML_IMAGE_str, &xml_image,
                            AttributeDesc::DiscreteColumnId::LONG_TEXT_str, &user_image,
                            AttributeDesc::DiscreteColumnId::ICON_pix,      &icon, -1);
        if (   (g_strcmp0 ("Q", xml_image) == 0)
            || (g_strcmp0 ("A", xml_image) == 0)
            || (g_strcmp0 ("E", xml_image) == 0))
        {
          GtkTreeIter pool_iter;

          gtk_tree_store_append (GTK_TREE_STORE (_status_model), &pool_iter, nullptr);
          gtk_tree_store_set (GTK_TREE_STORE (_status_model), &pool_iter,
                              AttributeDesc::DiscreteColumnId::XML_IMAGE_str, xml_image,
                              AttributeDesc::DiscreteColumnId::LONG_TEXT_str, user_image,
                              AttributeDesc::DiscreteColumnId::ICON_pix,      icon,
                              -1);
        }

        g_free (xml_image);
        g_free (user_image);
        g_object_unref (icon);
        iter_is_valid = gtk_tree_model_iter_next (desc->_discrete_model,
                                                  &iter);
      }
    }
  }

  return _status_model;
}
