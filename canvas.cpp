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

#include <goocanvas.h>

#include "canvas.hpp"

#define VALUE_INIT {0,{{0}}}

// --------------------------------------------------------------------------------
void Canvas::PutInTable (GooCanvasItem *table,
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
void Canvas::SetTableItemAttribute (GooCanvasItem *item,
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
void Canvas::SetTableItemAttribute (GooCanvasItem *item,
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
GooCanvasItem *Canvas::PutTextInTable (GooCanvasItem *table,
                                       gchar         *text,
                                       guint          row,
                                       guint          column)
{
  GooCanvasItem *item;

  item = goo_canvas_text_new (table,
                              text,
                              0.0, 0.0,
                              -1.0,
                              GTK_ANCHOR_NW,
                              "font", "Sans 14px", NULL);

  PutInTable (table,
              item,
              row, column);

  return item;
}

// --------------------------------------------------------------------------------
GooCanvasItem *Canvas::PutStockIconInTable (GooCanvasItem *table,
                                            gchar         *icon_name,
                                            guint          row,
                                            guint          column)
{
  GooCanvasItem *item;
  GdkPixbuf     *pixbuf;

  {
    GtkWidget *image = gtk_image_new ();

    g_object_ref_sink (image);
    pixbuf = gtk_widget_render_icon (image,
                                     icon_name,
                                     GTK_ICON_SIZE_BUTTON,
                                     NULL);
    g_object_unref (image);
  }

  item = goo_canvas_image_new (table,
                               pixbuf,
                               0.0,
                               0.0,
                               NULL);
  g_object_unref (pixbuf);

  PutInTable (table,
              item,
              row, column);

  return item;
}

// --------------------------------------------------------------------------------
GooCanvas *Canvas::CreatePrinterCanvas (GtkPrintContext *context)
{
  GooCanvas *canvas = GOO_CANVAS (goo_canvas_new ());

  g_object_set (G_OBJECT (canvas),
                "resolution-x", gtk_print_context_get_dpi_x (context),
                "resolution-y", gtk_print_context_get_dpi_y (context),
                "automatic-bounds", TRUE,
                "bounds-from-origin", FALSE,
                NULL);

  {
    gdouble        paper_w = gtk_print_context_get_width (context);
    cairo_matrix_t matrix;

    cairo_matrix_init_scale (&matrix,
                             paper_w/100.0,
                             paper_w/100.0);

    goo_canvas_item_set_transform (goo_canvas_get_root_item (canvas),
                                   &matrix);
  }

  return canvas;
}

// --------------------------------------------------------------------------------
void Canvas::FitToContext (GooCanvasItem   *item,
                           GtkPrintContext *context)
{
  gdouble    canvas_w;
  gdouble    canvas_h;
  gdouble    scale;
  GooCanvas *canvas  = goo_canvas_item_get_canvas (item);
  gdouble    paper_w = gtk_print_context_get_width (context);
  gdouble    paper_h = gtk_print_context_get_height (context);

  {
    GooCanvasBounds bounds;

    goo_canvas_item_get_bounds (item,
                                &bounds);
    canvas_w = bounds.x2 - bounds.x1;
    canvas_h = bounds.y2 - bounds.y1;
  }

  {
    gdouble canvas_dpi;
    gdouble printer_dpi;

    g_object_get (canvas,
                  "resolution-x", &canvas_dpi,
                  NULL);
    printer_dpi = gtk_print_context_get_dpi_x (context);

    scale = printer_dpi/canvas_dpi;
  }

  if (   (canvas_w*scale > paper_w)
      || (canvas_h*scale > paper_h))
  {
    cairo_matrix_t matrix;
    gdouble        x_scale = scale * paper_w/canvas_w;
    gdouble        y_scale = scale * paper_h/canvas_h;

    scale = MIN (x_scale, y_scale);

    goo_canvas_item_get_transform (item,
                                   &matrix);
    cairo_matrix_scale (&matrix,
                        scale,
                        scale);
    goo_canvas_item_set_transform (item,
                                   &matrix);
  }
}
