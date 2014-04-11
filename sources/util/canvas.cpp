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
  if (table)
  {
    SetTableItemAttribute (item, "row", row);
    SetTableItemAttribute (item, "column", column);
    SetTableItemAttribute (item, "x-align", 0.0);
    SetTableItemAttribute (item, "y-align", 0.0);
  }
}

// --------------------------------------------------------------------------------
void Canvas::SetTableItemAttribute (GooCanvasItem *item,
                                    const gchar   *attribute,
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
                                    const gchar   *attribute,
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
                                       const gchar   *text,
                                       guint          row,
                                       guint          column)
{
  GooCanvasItem *item = NULL;

  if (text)
  {
    item = goo_canvas_text_new (table,
                                text,
                                0.0, 0.0,
                                -1.0,
                                GTK_ANCHOR_NW,
                                "font", "Sans 14px", NULL);
    if (strchr (text, ' '))
    {
      g_object_set (G_OBJECT (item),
                    "wrap",      PANGO_WRAP_CHAR,
                    "ellipsize", PANGO_ELLIPSIZE_END,
                    NULL);
    }

    PutInTable (table,
                item,
                row, column);

    SetTableItemAttribute (item, "x-expand", 1U);
    SetTableItemAttribute (item, "x-fill",   1U);
    SetTableItemAttribute (item, "x-shrink", 1U);

    //SetTableItemAttribute (item, "y-expand", 1U);
    //SetTableItemAttribute (item, "y-fill", 1U);
    //SetTableItemAttribute (item, "y-shrink", 1U);
  }

  return item;
}

// --------------------------------------------------------------------------------
GooCanvasItem *Canvas::CreateIcon (GooCanvasItem *parent,
                                   const gchar   *icon_name,
                                   gdouble        x,
                                   gdouble        y)
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

  item = goo_canvas_image_new (parent,
                               pixbuf,
                               x,
                               y,
                               NULL);
  g_object_unref (pixbuf);

  return item;
}

// --------------------------------------------------------------------------------
GooCanvasItem *Canvas::PutStockIconInTable (GooCanvasItem *table,
                                            const gchar   *icon_name,
                                            guint          row,
                                            guint          column)
{
  GooCanvasItem *item = CreateIcon (table,
                                    icon_name,
                                    0.0,
                                    0.0);

  PutInTable (table,
              item,
              row, column);

  return item;
}

// --------------------------------------------------------------------------------
GooCanvasItem *Canvas::PutIconInTable (GooCanvasItem *table,
                                       gchar         *icon_name,
                                       guint          row,
                                       guint          column)
{
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (icon_name, NULL);

  GooCanvasItem *item = goo_canvas_image_new (table,
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
GooCanvasItem *Canvas::PutPixbufInTable (GooCanvasItem *table,
                                         GdkPixbuf     *pixbuf,
                                         guint          row,
                                         guint          column)
{
  GooCanvasItem *item = goo_canvas_image_new (table,
                                              pixbuf,
                                              0.0,
                                              0.0,
                                              NULL);

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
                "resolution-x",       gtk_print_context_get_dpi_x (context),
                "resolution-y",       gtk_print_context_get_dpi_y (context),
                "automatic-bounds",   TRUE,
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
gdouble Canvas::GetScaleToFit (GooCanvasItem   *item,
                               GtkPrintContext *context)
{
  gdouble    canvas_w;
  gdouble    scale;
  GooCanvas *canvas  = goo_canvas_item_get_canvas (item);
  gdouble    paper_w = gtk_print_context_get_width (context);

  {
    GooCanvasBounds bounds;

    goo_canvas_item_get_bounds (item,
                                &bounds);
    canvas_w = bounds.x2 - bounds.x1;
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

  if (canvas_w*scale > paper_w)
  {
    gdouble x_scale = scale * paper_w/canvas_w;

    scale = x_scale;
  }
  else
  {
    scale = 1.0;
  }

  return scale;
}

// --------------------------------------------------------------------------------
void Canvas::FitToContext (GooCanvasItem   *item,
                           GtkPrintContext *context)
{
  gdouble scale = GetScaleToFit (item, context);

  if (scale != 1.0)
  {
    cairo_matrix_t  matrix;
    GooCanvasBounds bounds;

    if (goo_canvas_item_get_transform (item,
                                       &matrix) == FALSE)
    {
      cairo_matrix_init_identity (&matrix);
    }

    goo_canvas_item_get_bounds (item,
                                &bounds);
    goo_canvas_convert_to_item_space (goo_canvas_item_get_canvas (item),
                                      item,
                                      &bounds.x1,
                                      &bounds.y1);

    cairo_matrix_scale (&matrix,
                        scale,
                        scale);

    cairo_matrix_translate (&matrix,
                            0.0,
                            bounds.y1/scale - bounds.y1);
    goo_canvas_item_set_transform (item,
                                   &matrix);
  }
}

// --------------------------------------------------------------------------------
void Canvas::Anchor (GooCanvasItem *item,
                     GooCanvasItem *to_bottom_of,
                     GooCanvasItem *to_right_of,
                     guint          space)
{
  GooCanvasBounds item_bounds;
  GooCanvasBounds to_bounds;
  gdouble         dx;
  gdouble         dy;

  goo_canvas_item_get_bounds (item,
                              &item_bounds);

  if (to_right_of)
  {
    goo_canvas_item_get_bounds (to_right_of,
                                &to_bounds);

    dx = to_bounds.x2 - item_bounds.x1 + space;
    dy = 0;

    goo_canvas_convert_to_item_space (goo_canvas_item_get_canvas (item),
                                      item,
                                      &dx,
                                      &dy);
    goo_canvas_item_translate (item,
                               dx,
                               0);
  }

  if (to_bottom_of)
  {
    goo_canvas_item_get_bounds (to_bottom_of,
                                &to_bounds);

    dx = 0;
    dy = to_bounds.y2 - item_bounds.y1 + space;

    goo_canvas_convert_to_item_space (goo_canvas_item_get_canvas (item),
                                      item,
                                      &dx,
                                      &dy);
    goo_canvas_item_translate (item,
                               0,
                               dy);
  }
}

// --------------------------------------------------------------------------------
void Canvas::VAlign (GooCanvasItem *item,
                     Alignment      item_alignment,
                     GooCanvasItem *with,
                     Alignment      with_alignment,
                     gdouble        offset)
{
  GooCanvasBounds item_bounds;
  GooCanvasBounds with_bounds;
  gdouble         dx = 0;
  gdouble         dy = 0;

  goo_canvas_item_get_bounds (item,
                              &item_bounds);
  goo_canvas_item_get_bounds (with,
                              &with_bounds);

  dx = GetDeltaAlign (item_bounds.x1,
                      item_bounds.x2,
                      item_alignment,
                      with_bounds.x1,
                      with_bounds.x2,
                      with_alignment);
  dx += offset;

  goo_canvas_convert_to_item_space (goo_canvas_item_get_canvas (item),
                                    item,
                                    &dx,
                                    &dy);
  goo_canvas_item_translate (item,
                             dx,
                             0);
}

// --------------------------------------------------------------------------------
void Canvas::HAlign (GooCanvasItem *item,
                     Alignment      item_alignment,
                     GooCanvasItem *with,
                     Alignment      with_alignment,
                     gdouble        offset)
{
  GooCanvasBounds item_bounds;
  GooCanvasBounds with_bounds;
  gdouble         dx = 0;
  gdouble         dy = 0;

  goo_canvas_item_get_bounds (item,
                              &item_bounds);
  goo_canvas_item_get_bounds (with,
                              &with_bounds);

  dy = GetDeltaAlign (item_bounds.y1,
                      item_bounds.y2,
                      item_alignment,
                      with_bounds.y1,
                      with_bounds.y2,
                      with_alignment);
  dy += offset;

  goo_canvas_convert_to_item_space (goo_canvas_item_get_canvas (item),
                                    item,
                                    &dx,
                                    &dy);
  goo_canvas_item_translate (item,
                             0,
                             dy);
}
// --------------------------------------------------------------------------------
void Canvas::NormalyzeDecimalNotation (gchar *string)
{
  gchar *decimal = strchr (string, ',');

  if (decimal)
  {
    *decimal = '.';
  }
}

// --------------------------------------------------------------------------------
gdouble Canvas::GetDeltaAlign (gdouble   item_1,
                               gdouble   item_2,
                               Alignment item_alignment,
                               gdouble   with_1,
                               gdouble   with_2,
                               Alignment with_alignment)
{
  gdouble delta       = 0.0;
  gdouble with_anchor = with_1;

  if (with_alignment == START)
  {
    with_anchor = with_1;
  }
  else if (with_alignment == MIDDLE)
  {
    with_anchor = (with_2 - with_1)/2;
  }
  else if (with_alignment == END)
  {
    with_anchor = with_2;
  }

  if (item_alignment == START)
  {
    delta = with_anchor - item_1;
  }
  else if (item_alignment == MIDDLE)
  {
    delta = with_anchor - (item_2 - item_1)/2;
  }
  else if (item_alignment == END)
  {
    delta = with_anchor - item_2;
  }

  return delta;
}
