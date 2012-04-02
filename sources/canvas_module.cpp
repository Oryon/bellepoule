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
  _canvas           = NULL;
  _drop_zones       = NULL;
  _target_drop_zone = NULL;
  _floating_object  = NULL;
  _dragging         = FALSE;
  _drag_text        = NULL;
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

// --------------------------------------------------------------------------------
void CanvasModule::EnableDragAndDrop ()
{
  GooCanvasItem *root = GetRootItem ();

  g_object_set (G_OBJECT (root),
                "pointer-events", GOO_CANVAS_EVENTS_VISIBLE,
                NULL);
  g_signal_connect (root, "motion_notify_event",
                    G_CALLBACK (on_motion_notify), this);
  g_signal_connect (root, "button_release_event",
                    G_CALLBACK (on_button_release), this);

}

// --------------------------------------------------------------------------------
void CanvasModule::SetObjectDropZone (Object        *object,
                                      GooCanvasItem *item,
                                      DropZone      *drop_zone)
{
  g_object_set_data (G_OBJECT (item),
                     "CanvasModule::drop_object",
                     object);
  drop_zone->SetData (drop_zone, "CanvasModule::canvas",
                      this);

  g_signal_connect (item, "button_press_event",
                    G_CALLBACK (on_button_press), drop_zone);
  g_signal_connect (item, "enter_notify_event",
                    G_CALLBACK (on_enter_object), drop_zone);
  g_signal_connect (item, "leave_notify_event",
                    G_CALLBACK (on_leave_object), drop_zone);
}

// --------------------------------------------------------------------------------
DropZone *CanvasModule::GetZoneAt (gint x,
                                   gint y)
{
  gdouble  vvalue;
  gdouble  hvalue;
  GSList  *current = _drop_zones;

  {
    GtkWidget     *window = _glade->GetWidget ("canvas_scrolled_window");
    GtkAdjustment *adjustment;

    adjustment = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (window));
    hvalue     = gtk_adjustment_get_value (adjustment);

    adjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (window));
    vvalue     = gtk_adjustment_get_value (adjustment);
  }


  while (current)
  {
    GooCanvasBounds  bounds;
    DropZone        *drop_zone = (DropZone *) current->data;

    drop_zone->GetBounds (&bounds);

    if (   (x > bounds.x1-hvalue) && (x < bounds.x2-hvalue)
           && (y > bounds.y1-vvalue) && (y < bounds.y2-vvalue))
    {
      return drop_zone;
    }

    current = g_slist_next (current);
  }

  return NULL;
}

// --------------------------------------------------------------------------------
gboolean CanvasModule::OnDragMotion (GtkWidget      *widget,
                                     GdkDragContext *drag_context,
                                     gint            x,
                                     gint            y,
                                     guint           time)
{
  if (DroppingIsForbidden ())
  {
    gdk_drag_status  (drag_context,
                      (GdkDragAction) 0,
                      time);
    return FALSE;
  }

  if (drag_context->targets)
  {
    GdkAtom  target_type;

    target_type = GDK_POINTER_TO_ATOM (g_list_nth_data (drag_context->targets,
                                                        0));

    gtk_drag_get_data (widget,
                       drag_context,
                       target_type,
                       time);

  }

  if (_target_drop_zone)
  {
    _target_drop_zone->Unfocus ();
    _target_drop_zone = NULL;
  }

  {
    DropZone *drop_zone = GetZoneAt (x,
                                     y);

    if (drop_zone)
    {
      drop_zone->Focus ();

      if (_floating_object)
      {
        //ylr: Player::AttributeId  attr_id  ("availability");
        //ylr: Attribute           *attr = _floating_object->GetAttribute (&attr_id);

        //ylr: if (attr && (strcmp (attr->GetStrValue (), "Free") == 0))
        {
          _target_drop_zone = drop_zone;
          gdk_drag_status  (drag_context,
                            GDK_ACTION_COPY,
                            time);
          return TRUE;
        }
      }
    }
  }

  gdk_drag_status  (drag_context,
                    (GdkDragAction) 0,
                    time);
  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean CanvasModule::OnDragDrop (GtkWidget      *widget,
                                   GdkDragContext *drag_context,
                                   gint            x,
                                   gint            y,
                                   guint           time)
{
  gboolean result = FALSE;

  if (drag_context->targets)
  {
    GdkAtom  target_type;

    target_type = GDK_POINTER_TO_ATOM (g_list_nth_data (drag_context->targets,
                                                        0));

    gtk_drag_get_data (widget,
                       drag_context,
                       target_type,
                       time);

  }

  if (_floating_object && _target_drop_zone)
  {
    _target_drop_zone->AddObject (_floating_object);
    DropObject (_floating_object,
                _source_drop_zone,
                _target_drop_zone);

    _target_drop_zone->Unfocus ();
    _target_drop_zone = NULL;

    result = TRUE;
  }

  gtk_drag_finish (drag_context,
                   result,
                   FALSE,
                   time);

  return result;
}

// --------------------------------------------------------------------------------
void CanvasModule::OnDragDataReceived (GtkWidget        *widget,
                                       GdkDragContext   *drag_context,
                                       gint              x,
                                       gint              y,
                                       GtkSelectionData *data,
                                       guint             info,
                                       guint             time)
{
  if (data && (data->length >= 0))
  {
    if (info == INT_TARGET)
    {
      guint32 *ref = (guint32 *) data->data;

      _floating_object = GetDropObjectFromRef (*ref);
    }
  }
}

// --------------------------------------------------------------------------------
void CanvasModule::OnDragLeave (GtkWidget      *widget,
                                GdkDragContext *drag_context,
                                guint           time)
{
  if (_target_drop_zone)
  {
    _target_drop_zone->Unfocus ();
  }
}

// --------------------------------------------------------------------------------
gboolean CanvasModule::on_button_press (GooCanvasItem  *item,
                                        GooCanvasItem  *target,
                                        GdkEventButton *event,
                                        DropZone       *drop_zone)
{
  CanvasModule *canvas = (CanvasModule*) drop_zone->GetPtrData (drop_zone, "CanvasModule::canvas");

  if (canvas)
  {
    return canvas->OnButtonPress (item,
                                  target,
                                  event,
                                  drop_zone);
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean CanvasModule::OnButtonPress (GooCanvasItem  *item,
                                      GooCanvasItem  *target,
                                      GdkEventButton *event,
                                      DropZone       *drop_zone)
{
  if (DroppingIsForbidden ())
  {
    return FALSE;
  }

  if (   (event->button == 1)
         && (event->type == GDK_BUTTON_PRESS))
  {
    _floating_object = (Player *) g_object_get_data (G_OBJECT (item),
                                                     "CanvasModule::drop_object");

    _drag_x = event->x;
    _drag_y = event->y;

    goo_canvas_convert_from_item_space  (GetCanvas (),
                                         item,
                                         &_drag_x,
                                         &_drag_y);

    {
      GooCanvasBounds  bounds;
      GString         *string = GetFloatingImage (_floating_object);

      goo_canvas_item_get_bounds (item,
                                  &bounds);

      _drag_text = goo_canvas_text_new (GetRootItem (),
                                        string->str,
                                        //bounds.x1,
                                        //bounds.y1,
                                        _drag_x,
                                        _drag_y,
                                        -1,
                                        GTK_ANCHOR_NW,
                                        "font", "Sans Bold 14px", NULL);
      g_string_free (string,
                     TRUE);

    }

    SetCursor (GDK_FLEUR);

    _dragging = TRUE;
    _source_drop_zone = drop_zone;
    _target_drop_zone = drop_zone;

    {
      drop_zone->Focus ();
    }

    DragObject (_floating_object,
                _source_drop_zone);

    MakeDirty ();
    return TRUE;
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean CanvasModule::on_button_release (GooCanvasItem  *item,
                                          GooCanvasItem  *target,
                                          GdkEventButton *event,
                                          CanvasModule  *canvas)
{
  if (canvas)
  {
    return canvas->OnButtonRelease (item,
                                    target,
                                    event);
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean CanvasModule::OnButtonRelease (GooCanvasItem  *item,
                                        GooCanvasItem  *target,
                                        GdkEventButton *event)
{
  if (_dragging)
  {
    if (_target_drop_zone)
    {
      _target_drop_zone->Unfocus ();
    }

    DropObject (_floating_object,
                _source_drop_zone,
                _target_drop_zone);

    _dragging = FALSE;

    goo_canvas_item_remove (_drag_text);
    _drag_text = NULL;

    _target_drop_zone = NULL;
    _source_drop_zone = NULL;

    ResetCursor ();
    MakeDirty ();
  }

  return TRUE;
}

// --------------------------------------------------------------------------------
gboolean CanvasModule::on_motion_notify (GooCanvasItem  *item,
                                         GooCanvasItem  *target,
                                         GdkEventButton *event,
                                         CanvasModule  *canvas)
{
  if (canvas)
  {
    canvas->OnMotionNotify (item,
                            target,
                            event);
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean CanvasModule::OnMotionNotify (GooCanvasItem  *item,
                                       GooCanvasItem  *target,
                                       GdkEventButton *event)
{
  if (_dragging && (event->state & GDK_BUTTON1_MASK))
  {
    {
      gdouble new_x = event->x_root;
      gdouble new_y = event->y_root;

      goo_canvas_item_translate (_drag_text,
                                 new_x - _drag_x,
                                 new_y - _drag_y);

      _drag_x = new_x;
      _drag_y = new_y;
    }

    {
      DropZone *drop_zone = GetZoneAt (_drag_x,
                                       _drag_y);

      if (drop_zone)
      {
        drop_zone->Focus ();
        _target_drop_zone = drop_zone;
      }
      else if (_target_drop_zone)
      {
        _target_drop_zone->Unfocus ();
        _target_drop_zone = NULL;
      }
    }

    SetCursor (GDK_FLEUR);
  }

  return TRUE;
}

// --------------------------------------------------------------------------------
Object *CanvasModule::GetDropObjectFromRef (guint32 ref)
{
  return NULL;
}

// --------------------------------------------------------------------------------
void CanvasModule::DragObject (Object   *object,
                               DropZone *from_zone)
{
}

// --------------------------------------------------------------------------------
void CanvasModule::DropObject (Object   *object,
                               DropZone *source_zone,
                               DropZone *target_zone)
{
}

// --------------------------------------------------------------------------------
gboolean CanvasModule::DroppingIsForbidden ()
{
  return TRUE;
}

// --------------------------------------------------------------------------------
GString *CanvasModule::GetFloatingImage (Object *floating_object)
{
  return g_string_new ("???");
}

// --------------------------------------------------------------------------------
gboolean CanvasModule::on_enter_object (GooCanvasItem  *item,
                                        GooCanvasItem  *target,
                                        GdkEventButton *event,
                                        DropZone       *zone)
{
  CanvasModule *module = (CanvasModule*) zone->GetPtrData (zone, "CanvasModule::canvas");

  if (module)
  {
    module->SetCursor (GDK_FLEUR);
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean CanvasModule::on_leave_object (GooCanvasItem  *item,
                                        GooCanvasItem  *target,
                                        GdkEventButton *event,
                                        DropZone       *zone)
{
  CanvasModule *module = (CanvasModule*) zone->GetPtrData (zone, "CanvasModule::canvas");

  if (module)
  {
    module->ResetCursor ();
  }

  return FALSE;
}
