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

#include "canvas.hpp"
#include "module.hpp"

class CanvasModule : public Module
{
  protected:
    CanvasModule (gchar *glade_file,
                  gchar *root = NULL);
    virtual ~CanvasModule ();

    static void WipeItem (GooCanvasItem *item);

    virtual void Wipe ();
    virtual void OnPlugged ();
    virtual void OnUnPlugged ();

    GooCanvas *GetCanvas ();
    GooCanvasItem *GetRootItem ();

  private:
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
};

#endif
