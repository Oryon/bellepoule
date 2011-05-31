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

#ifndef canvas_hpp
#define canvas_hpp

#include <gtk/gtk.h>
#include <goocanvas.h>

#include "object.hpp"

class Canvas
{
  public:
    static void PutInTable (GooCanvasItem *table,
                            GooCanvasItem *item,
                            guint          row,
                            guint          column);

    static GooCanvasItem *PutTextInTable (GooCanvasItem *table,
                                          const gchar   *text,
                                          guint          row,
                                          guint          column);

    static GooCanvasItem *CreateIcon (GooCanvasItem *parent,
                                      const gchar   *icon_name,
                                      gdouble        x,
                                      gdouble        y);

    static GooCanvasItem *PutIconInTable (GooCanvasItem *table,
                                          gchar         *icon_name,
                                          guint          row,
                                          guint          column);

    static GooCanvasItem *PutStockIconInTable (GooCanvasItem *table,
                                               const gchar   *icon_name,
                                               guint          row,
                                               guint          column);

    static void SetTableItemAttribute (GooCanvasItem *item,
                                       const gchar   *attribute,
                                       guint          value);

    static void SetTableItemAttribute (GooCanvasItem *item,
                                       const gchar   *attribute,
                                       gdouble        value);

    static void Anchor (GooCanvasItem *item,
                        GooCanvasItem *to_bottom_of,
                        GooCanvasItem *to_right_of,
                        guint          space = 0);

    static void Align (GooCanvasItem *item,
                       GooCanvasItem *with_left_of,
                       GooCanvasItem *with_top_of,
                       gdouble        offset = 0.0);

    static GooCanvas *CreatePrinterCanvas (GtkPrintContext *context);

    static gdouble GetScaleToFit (GooCanvasItem   *item,
                                  GtkPrintContext *context);

    static void FitToContext (GooCanvasItem   *item,
                              GtkPrintContext *context);
};

#endif
