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

#pragma once

#include <gtk/gtk.h>
#include <goocanvas.h>

#include "module.hpp"
#include "canvas.hpp"

class Player;
class DropZone;

class CanvasModule : public Module
{
  public:
    virtual void UnPlug ();

  protected:
    CanvasModule (const gchar *glade_file,
                  const gchar *root = NULL);
    virtual ~CanvasModule ();

    static void WipeItem (GooCanvasItem *item);

    virtual void Wipe ();
    virtual void OnPlugged ();
    virtual void OnUnPlugged ();

    GooCanvas *CreateCanvas ();
    GooCanvas *GetCanvas ();
    GooCanvasItem *GetRootItem ();

    GooCanvasItem *GetPlayerImage (GooCanvasItem *parent_item,
                                   const gchar   *common_markup,
                                   Player        *player,
                                   Filter        *filter,
                                   ...);

    void SetZoomer (GtkRange *zoomer);

    void ZoomTo (gdouble scale);

    void FreezeZoomer ();

    void RestoreZoomFactor ();

  protected:
    virtual guint PreparePrint (GtkPrintOperation *operation,
                                GtkPrintContext   *context);
    virtual void DrawPage (GtkPrintOperation *operation,
                           GtkPrintContext   *context,
                           gint               page_nr);
    virtual gboolean PreparePreview (GtkPrintOperation        *operation,
                                     GtkPrintOperationPreview *preview,
                                     GtkPrintContext          *context,
                                     GtkWindow                *parent);
    virtual void OnEndPrint (GtkPrintOperation *operation,
                             GtkPrintContext   *context);

  protected:
    GSList *_drop_zones;

    virtual gboolean DroppingIsAllowed (Object   *floating_object,
                                        DropZone *in_zone);

  public:
    void EnableDndOnCanvas ();

    void SetObjectDropZone (Object        *object,
                            GooCanvasItem *item,
                            DropZone      *drop_zone);

  private:
    virtual void DragObject (Object   *object,
                             DropZone *from_zone);

    virtual void DropObject (Object   *object,
                             DropZone *source_zone,
                             DropZone *target_zone);

    virtual gboolean DragingIsForbidden (Object *object);

    virtual GString *GetFloatingImage (Object *floating_object);

  private:
    GooCanvas         *_canvas;
    GtkScrolledWindow *_scrolled_window;
    GtkRange          *_zoomer;
    gdouble            _zoom_factor;
    gdouble            _h_adj;
    gdouble            _v_adj;
    gboolean           _dragging;
    GooCanvasItem     *_drag_text;
    DropZone          *_source_drop_zone;
    DropZone          *_target_drop_zone;
    gdouble            _drag_x;
    gdouble            _drag_y;

    void OnZoom (gdouble value);

    DropZone *GetZoneAt (gint x,
                         gint y);

    gboolean OnDragMotion (GtkWidget      *widget,
                           GdkDragContext *drag_context,
                           gint            x,
                           gint            y,
                           guint           time);

    void OnDragLeave (GtkWidget      *widget,
                      GdkDragContext *drag_context,
                      guint           time);

    gboolean OnDragDrop (GtkWidget      *widget,
                         GdkDragContext *drag_context,
                         gint            x,
                         gint            y,
                         guint           time);

    virtual gboolean OnButtonPress (GooCanvasItem  *item,
                                    GooCanvasItem  *target,
                                    GdkEventButton *event,
                                    DropZone       *drop_zone);

    virtual gboolean OnButtonRelease (GooCanvasItem  *item,
                                      GooCanvasItem  *target,
                                      GdkEventButton *event);

    virtual gboolean OnMotionNotify (GooCanvasItem  *item,
                                     GooCanvasItem  *target,
                                     GdkEventMotion *event);

  private:
    static gboolean on_button_press (GooCanvasItem  *item,
                                     GooCanvasItem  *target,
                                     GdkEventButton *event,
                                     DropZone       *drop_zone);
    static gboolean on_button_release (GooCanvasItem  *item,
                                       GooCanvasItem  *target,
                                       GdkEventButton *event,
                                       CanvasModule   *module);
    static gboolean on_motion_notify (GooCanvasItem  *item,
                                      GooCanvasItem  *target,
                                      GdkEventMotion *event,
                                      CanvasModule   *module);
    static gboolean on_enter_object (GooCanvasItem  *item,
                                     GooCanvasItem  *target,
                                     GdkEventButton *event,
                                     DropZone       *zone);
    static gboolean on_leave_object (GooCanvasItem  *item,
                                     GooCanvasItem  *target,
                                     GdkEventButton *event,
                                     DropZone       *zone);
    static void on_hadjustment_changed (GtkAdjustment *adjustment,
                                        CanvasModule  *canvas_module);
    static void on_vadjustment_changed (GtkAdjustment *adjustment,
                                        CanvasModule  *canvas_module);
    static void on_zoom_changed (GtkRange     *range,
                                 CanvasModule *canvas_module);
};
