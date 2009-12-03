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

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "schedule.hpp"

#include "canvas_module.hpp"

#define VALUE_INIT {0,{{0}}}

// --------------------------------------------------------------------------------
CanvasModule_c::CanvasModule_c (gchar *glade_file,
                                gchar *root)
: Module_c (glade_file,
            root)
{
  _canvas = NULL;
}

// --------------------------------------------------------------------------------
CanvasModule_c::~CanvasModule_c ()
{
  Wipe ();

  UnPlug ();
}

// --------------------------------------------------------------------------------
void CanvasModule_c::OnPlugged ()
{
  if (_canvas == NULL)
  {
    GtkWidget *view_port = _glade->GetWidget ("canvas_scrolled_window");

    _canvas = GOO_CANVAS (goo_canvas_new ());

    g_object_set (G_OBJECT (_canvas),
                  "automatic-bounds", TRUE,
                  "bounds-padding", 10.0,
                  "bounds-from-origin", FALSE,
                  NULL);

    gtk_container_add (GTK_CONTAINER (view_port), GTK_WIDGET (_canvas));
    gtk_widget_show (GTK_WIDGET (_canvas));
  }
}

// --------------------------------------------------------------------------------
void CanvasModule_c::OnUnPlugged ()
{
  if (_canvas)
  {
    gtk_widget_destroy (GTK_WIDGET (_canvas));
    _canvas = NULL;
  }
}

// --------------------------------------------------------------------------------
void CanvasModule_c::PutInTable (GooCanvasItem *table,
                                 GooCanvasItem *item,
                                 guint          row,
                                 guint          column)
{
  SetTableItemAttribute (item, "row", row);
  SetTableItemAttribute (item, "column", column);
  SetTableItemAttribute (item, "x-align", 0.0);
  SetTableItemAttribute (item, "y-align", 0.0);
}

// --------------------------------------------------------------------------------
void CanvasModule_c::SetTableItemAttribute (GooCanvasItem *item,
                                            gchar         *attribute,
                                            gdouble        value)
{
  GooCanvasItem *parent = goo_canvas_item_get_parent (item);

  if (parent)
  {
    GValue gvalue = VALUE_INIT;

    g_value_init (&gvalue, G_TYPE_DOUBLE);
    g_value_set_double (&gvalue, value);
    goo_canvas_item_set_child_property (parent, item, attribute, &gvalue);
  }
}

// --------------------------------------------------------------------------------
void CanvasModule_c::SetTableItemAttribute (GooCanvasItem *item,
                                            gchar         *attribute,
                                            guint          value)
{
  GooCanvasItem *parent = goo_canvas_item_get_parent (item);

  if (parent)
  {
    GValue gvalue = VALUE_INIT;

    g_value_init (&gvalue, G_TYPE_UINT);
    g_value_set_uint (&gvalue, value);
    goo_canvas_item_set_child_property (parent, item, attribute, &gvalue);
  }
}

// --------------------------------------------------------------------------------
GooCanvasItem *CanvasModule_c::PutTextInTable (GooCanvasItem *table,
                                               gchar         *text,
                                               guint          row,
                                               guint          column)
{
  GooCanvasItem *item;

  item = goo_canvas_text_new (table,
                              text,
                              0, 0,
                              -1,
                              GTK_ANCHOR_NW,
                              "font", "Sans 14px", NULL);

  PutInTable (table,
              item,
              row, column);

  return item;
}

// --------------------------------------------------------------------------------
GooCanvasItem *CanvasModule_c::PutStockIconInTable (GooCanvasItem *table,
                                                    gchar         *icon_name,
                                                    guint          row,
                                                    guint          column)
{
  GooCanvasItem *item;
  GtkWidget     *image = gtk_image_new ();
  GdkPixbuf     *pixbuf;

  g_object_ref_sink (image);
  pixbuf = gtk_widget_render_icon (image,
                                   icon_name,
                                   GTK_ICON_SIZE_BUTTON,
                                   NULL);

  item = goo_canvas_image_new (table,
                               pixbuf,
                               0.0,
                               0.0,
                               NULL);

  PutInTable (table,
              item,
              row, column);

  g_object_unref (image);

  return item;
}

// --------------------------------------------------------------------------------
GooCanvas *CanvasModule_c::GetCanvas ()
{
  return _canvas;
}

// --------------------------------------------------------------------------------
GooCanvasItem *CanvasModule_c::GetRootItem ()
{
  if (_canvas)
  {
    return (goo_canvas_get_root_item (_canvas));
  }
  else
  {
    return NULL;
  }
}

// --------------------------------------------------------------------------------
void CanvasModule_c::Wipe ()
{
  WipeItem (GetRootItem ());
}

// --------------------------------------------------------------------------------
void CanvasModule_c::WipeItem (GooCanvasItem *from)
{
  if (from)
  {
    guint nb_child = goo_canvas_item_get_n_children (from);

    for (guint i = 0; i < nb_child; i++)
    {
      GooCanvasItem *child;

      child = goo_canvas_item_get_child (from,
                                         0);
      if (child)
      {
        goo_canvas_item_remove (child);
      }
    }
    goo_canvas_item_remove (from);
  }
}

// --------------------------------------------------------------------------------
void CanvasModule_c::Print ()
{
  GtkPrintOperation *operation;
  GtkPrintSettings  *settings;
  GError            *error = NULL;

  operation = gtk_print_operation_new ();

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
void CanvasModule_c::OnBeginPrint (GtkPrintOperation *operation,
                                   GtkPrintContext   *context)
{
  GooCanvasBounds bounds;
  gdouble         canvas_w;
  gdouble         canvas_h;
  gdouble         paper_w = gtk_print_context_get_width (context);
  gdouble         paper_h = gtk_print_context_get_height (context);

  goo_canvas_item_get_bounds (GetRootItem (),
                              &bounds);
  canvas_w = bounds.x2 -bounds.x1;
  canvas_h = bounds.y2 -bounds.y1;

  {
    cairo_t *cr;
    gdouble  x_factor = paper_w / canvas_w;
    gdouble  y_factor = paper_h / canvas_h;
  }

  gtk_print_operation_set_n_pages (operation,
                                   1);
}

// --------------------------------------------------------------------------------
void CanvasModule_c::OnDrawPage (GtkPrintOperation *operation,
                                 GtkPrintContext   *context,
                                 gint               page_nr)
{
  GooCanvasBounds  bounds;
  gdouble          canvas_w;
  gdouble          canvas_h;
  gdouble          paper_w = gtk_print_context_get_width (context);
  gdouble          paper_h = gtk_print_context_get_height (context);
  cairo_t         *cr      = gtk_print_context_get_cairo_context (context);
  gdouble          scale;

  goo_canvas_item_get_bounds (GetRootItem (),
                              &bounds);
  canvas_w = bounds.x2 - bounds.x1;
  canvas_h = bounds.y2 - bounds.y1;

  // From canvas space -> printer space
  {
    gdouble canvas_dpi;
    gdouble printer_dpi;

    g_object_get (G_OBJECT (GetCanvas ()),
                  "resolution-x", &canvas_dpi,
                  NULL);
    printer_dpi = gtk_print_context_get_dpi_x (context);

    scale = printer_dpi/canvas_dpi;
  }

  if (   (canvas_w * scale > paper_w)
      || (canvas_h * scale > paper_h))
  {
    gdouble x_scale = paper_w / canvas_w;
    gdouble y_scale = paper_h / canvas_h;

    scale = MIN (x_scale, y_scale);
  }

  cairo_translate (cr,
                   -bounds.x1 * scale,
                   -bounds.y1 * scale);

  cairo_scale (cr,
               scale,
               scale);

  goo_canvas_render (GetCanvas (),
                     cr,
                     NULL,
                     1.0);

  cairo_show_page (cr);
  cairo_destroy (cr);
}

// --------------------------------------------------------------------------------
gboolean CanvasModule_c::OnPreview (GtkPrintOperation        *operation,
                                    GtkPrintOperationPreview *preview,
                                    GtkPrintContext          *context,
                                    GtkWindow                *parent)
{
  return TRUE;
}

// --------------------------------------------------------------------------------
void CanvasModule_c::OnEndPrint (GtkPrintOperation *operation,
                                 GtkPrintContext   *context)
{
}

// --------------------------------------------------------------------------------
void CanvasModule_c::on_begin_print (GtkPrintOperation *operation,
                                     GtkPrintContext   *context,
                                     CanvasModule_c    *canvas)
{
  canvas->OnBeginPrint (operation,
                        context);
}

// --------------------------------------------------------------------------------
void CanvasModule_c::on_draw_page (GtkPrintOperation *operation,
                                   GtkPrintContext   *context,
                                   gint               page_nr,
                                   CanvasModule_c    *canvas)
{
  canvas->OnDrawPage (operation,
                      context,
                      page_nr);
}

// --------------------------------------------------------------------------------
gboolean CanvasModule_c::on_preview (GtkPrintOperation        *operation,
                                     GtkPrintOperationPreview *preview,
                                     GtkPrintContext          *context,
                                     GtkWindow                *parent,
                                     CanvasModule_c           *canvas)
{
  return canvas->OnPreview (operation,
                            preview,
                            context,
                            parent);

#if 0
  GtkPrintSettings *settings;
  GtkWidget *window, *close, *page, *hbox, *vbox, *da;
  gdouble width, height;
  cairo_t *cr;
  Pool_c *pool = (Pool_c *) user_data;
  GooCanvas *canvas = goo_canvas_item_get_canvas (pool->_root_item);

  settings = gtk_print_operation_get_print_settings (operation);

  width = 200;
  height = 300;
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  //gtk_window_set_transient_for (GTK_WINDOW (window), 
  //GTK_WINDOW (main_window));
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox,
                      FALSE, FALSE, 0);
  page = gtk_spin_button_new_with_range (1, 100, 1);
  gtk_box_pack_start (GTK_BOX (hbox), page, FALSE, FALSE, 0);

  close = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  gtk_box_pack_start (GTK_BOX (hbox), close, FALSE, FALSE, 0);

  da = gtk_drawing_area_new ();
  gtk_widget_set_size_request (GTK_WIDGET (da), width, height);
  gtk_box_pack_start (GTK_BOX (vbox), da, TRUE, TRUE, 0);

  gtk_widget_set_double_buffered (da, FALSE);

  gtk_widget_realize (da);

  cr = gtk_print_context_get_cairo_context (context);
  goo_canvas_render (canvas,
                     cr,
                     NULL,
                     1.0);

  cairo_show_page (cr);
  cairo_destroy (cr);

  g_signal_connect (page, "value-changed", 
                    G_CALLBACK (update_page), NULL);
  g_signal_connect_swapped (close, "clicked", 
                            G_CALLBACK (gtk_widget_destroy), window);

  g_signal_connect (preview, "ready",
                    G_CALLBACK (preview_ready), NULL);
  g_signal_connect (preview, "got-page-size",
                    G_CALLBACK (preview_got_page_size), NULL);

  g_signal_connect (window, "destroy", 
                    G_CALLBACK (preview_destroy), NULL);

  gtk_widget_show_all (window);
#endif
}

// --------------------------------------------------------------------------------
void CanvasModule_c::on_end_print (GtkPrintOperation *operation,
                                   GtkPrintContext   *context,
                                   CanvasModule_c    *canvas)
{
  canvas->OnEndPrint (operation,
                      context);
}
