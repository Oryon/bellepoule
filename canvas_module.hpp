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

#ifndef canvas_module_hpp
#define canvas_module_hpp

#include <gtk/gtk.h>
#include <goocanvas.h>

#include "module.hpp"

class CanvasModule_c : public Module_c
{
  public:
    void Print ();

  protected:
    CanvasModule_c (gchar *glade_file,
                    gchar *root = NULL);
    virtual ~CanvasModule_c ();

    static void PutInTable (GooCanvasItem *table,
                            GooCanvasItem *item,
                            guint          row,
                            guint          column);

    static GooCanvasItem *PutTextInTable (GooCanvasItem *table,
                                          gchar         *text,
                                          guint          row,
                                          guint          column);

    static GooCanvasItem *PutStockIconInTable (GooCanvasItem *table,
                                               gchar         *icon_name,
                                               guint          row,
                                               guint          column);

    static void SetTableItemAttribute (GooCanvasItem *item,
                                       gchar         *attribute,
                                       guint          value);

    static void SetTableItemAttribute (GooCanvasItem *item,
                                       gchar         *attribute,
                                       gdouble        value);

    GooCanvas *GetCanvas ();
    GooCanvasItem *GetRootItem ();
    virtual void Wipe ();
    virtual void OnPlugged ();
    virtual void OnUnPlugged ();


  protected:
    virtual void OnBeginPrint (GtkPrintOperation *operation,
                               GtkPrintContext   *context);
    virtual void OnDrawPage (GtkPrintOperation *operation,
                             GtkPrintContext   *context,
                             gint               page_nr);
    virtual gboolean OnPreview (GtkPrintOperation        *operation,
                                GtkPrintOperationPreview *preview,
                                GtkPrintContext          *context,
                                GtkWindow                *parent);
    virtual void OnEndPrint (GtkPrintOperation *operation,
                             GtkPrintContext   *context);

  private:
    GooCanvas *_canvas;

    static void on_begin_print (GtkPrintOperation *operation,
                                GtkPrintContext   *context,
                                CanvasModule_c    *canvas);
    static gboolean on_preview (GtkPrintOperation        *operation,
                                GtkPrintOperationPreview *preview,
                                GtkPrintContext          *context,
                                GtkWindow                *parent,
                                CanvasModule_c           *canvas);
    static void on_draw_page (GtkPrintOperation *operation,
                              GtkPrintContext   *context,
                              gint               page_nr,
                              CanvasModule_c    *canvas);
    static void on_end_print (GtkPrintOperation *operation,
                              GtkPrintContext   *context,
                              CanvasModule_c    *canvas);
};

#endif
