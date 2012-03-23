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

// --------------------------------------------------------------------------------
CanvasModule::CanvasModule (const gchar *glade_file,
                            const gchar *root)
: Module (glade_file,
          root)
{
  _canvas = NULL;
}

// --------------------------------------------------------------------------------
CanvasModule::~CanvasModule ()
{
  Wipe ();

  UnPlug ();
}

// --------------------------------------------------------------------------------
void CanvasModule::OnPlugged ()
{
  if (_canvas == NULL)
  {
    GtkWidget *view_port = _glade->GetWidget ("canvas_scrolled_window");

    _canvas = CreateCanvas ();

    gtk_container_add (GTK_CONTAINER (view_port), GTK_WIDGET (_canvas));
    gtk_widget_show_all (GTK_WIDGET (_canvas));
  }
}

// --------------------------------------------------------------------------------
void CanvasModule::OnUnPlugged ()
{
  if (_canvas)
  {
    gtk_widget_destroy (GTK_WIDGET (_canvas));
    _canvas = NULL;
  }
}

// --------------------------------------------------------------------------------
GooCanvas *CanvasModule::CreateCanvas ()
{
  GooCanvas *canvas = GOO_CANVAS (goo_canvas_new ());

  g_object_set (G_OBJECT (canvas),
                "automatic-bounds", TRUE,
                "bounds-padding", 10.0,
                "bounds-from-origin", FALSE,
                NULL);

  return canvas;
}

// --------------------------------------------------------------------------------
GooCanvas *CanvasModule::GetCanvas ()
{
  return _canvas;
}

// --------------------------------------------------------------------------------
GooCanvasItem *CanvasModule::GetRootItem ()
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
void CanvasModule::Wipe ()
{
  GooCanvasItem *root = GetRootItem ();

  if (root)
  {
    guint nb_child = goo_canvas_item_get_n_children (root);

    for (guint i = 0; i < nb_child; i++)
    {
      GooCanvasItem *child;

      child = goo_canvas_item_get_child (root,
                                         0);
      if (child)
      {
        goo_canvas_item_remove (child);
      }
    }
    goo_canvas_item_remove (root);
  }
}

// --------------------------------------------------------------------------------
void CanvasModule::WipeItem (GooCanvasItem *item)
{
  if (item)
  {
    goo_canvas_item_remove (item);
  }
}

// --------------------------------------------------------------------------------
void CanvasModule::OnBeginPrint (GtkPrintOperation *operation,
                                 GtkPrintContext   *context)
{
  gtk_print_operation_set_n_pages (operation,
                                   1);
}

// --------------------------------------------------------------------------------
void CanvasModule::OnDrawPage (GtkPrintOperation *operation,
                               GtkPrintContext   *context,
                               gint               page_nr)
{
  cairo_t   *cr     = gtk_print_context_get_cairo_context (context);
  GooCanvas *canvas = (GooCanvas *) g_object_get_data (G_OBJECT (operation), "operation_canvas");


  if (canvas == NULL)
  {
    canvas = _canvas;

    Module::OnDrawPage (operation,
                        context,
                        page_nr);
  }

  cairo_save (cr);

  //if (GTK_WIDGET_DRAWABLE (GTK_WIDGET (canvas)))
  {
    gdouble scale;
    gdouble canvas_x;
    gdouble canvas_y;
    gdouble canvas_w;
    gdouble canvas_h;
    gdouble paper_w  = gtk_print_context_get_width (context);
    gdouble paper_h  = gtk_print_context_get_height (context);
    gdouble header_h = (PRINT_HEADER_HEIGHT+2) * paper_w  / 100;
    gdouble footer_h = 2 * paper_w  / 100;

    {
      GooCanvasBounds bounds;

      goo_canvas_item_get_bounds (goo_canvas_get_root_item (canvas),
                                  &bounds);
      canvas_x = bounds.x1;
      canvas_y = bounds.y1;
      canvas_w = bounds.x2 - bounds.x1;
      canvas_h = bounds.y2 - bounds.y1;
    }

    {
      gdouble canvas_dpi;
      gdouble printer_dpi;

      g_object_get (G_OBJECT (canvas),
                    "resolution-x", &canvas_dpi,
                    NULL);
      printer_dpi = gtk_print_context_get_dpi_x (context);

      scale = printer_dpi/canvas_dpi;
    }

    if (   (canvas_w*scale > paper_w)
        || (canvas_h*scale + (header_h+footer_h) > paper_h))
    {
      gdouble x_scale = paper_w / canvas_w;
      gdouble y_scale = paper_h / (canvas_h + (header_h+footer_h)/scale);

      scale = MIN (x_scale, y_scale);
    }

    cairo_translate (cr,
                     -canvas_x*scale,
                     -canvas_y*scale + header_h);
    cairo_scale (cr,
                 scale,
                 scale);

    goo_canvas_render (canvas,
                       cr,
                       NULL,
                       1.0);
  }
  cairo_restore (cr);
}

// --------------------------------------------------------------------------------
gboolean CanvasModule::OnPreview (GtkPrintOperation        *operation,
                                  GtkPrintOperationPreview *preview,
                                  GtkPrintContext          *context,
                                  GtkWindow                *parent)
{
  GtkWidget *window, *close, *page, *hbox, *vbox;
  cairo_t *cr;

  {
    cairo_surface_t *surface;

    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                          300,
                                          300);
    cr = cairo_create (surface);
  }

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox,
                      FALSE, FALSE, 0);
  page = gtk_spin_button_new_with_range (1, 100, 1);
  gtk_box_pack_start (GTK_BOX (hbox), page, FALSE, FALSE, 0);

  close = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  gtk_box_pack_start (GTK_BOX (hbox), close, FALSE, FALSE, 0);

  gtk_print_context_set_cairo_context (context,
                                       cr,
                                       100.0,
                                       100.0);

  goo_canvas_render (_canvas,
                     cr,
                     NULL,
                     1.0);

  cairo_show_page (cr);

#if 0
  g_signal_connect (page, "value-changed",
                    G_CALLBACK (update_page), NULL);
  g_signal_connect_swapped (close, "clicked",
                            G_CALLBACK (gtk_widget_destroy), window);

#endif

  gtk_widget_show_all (window);

  return TRUE;
}

// --------------------------------------------------------------------------------
void CanvasModule::OnEndPrint (GtkPrintOperation *operation,
                               GtkPrintContext   *context)
{
}
