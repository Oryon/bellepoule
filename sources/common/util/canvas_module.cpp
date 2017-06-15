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

#include "util/player.hpp"

#include "canvas_module.hpp"

// --------------------------------------------------------------------------------
CanvasModule::CanvasModule (const gchar *glade_file,
                            const gchar *root)
  : Module (glade_file, root)
{
  _canvas           = NULL;
  _scrolled_window  = NULL;
  _zoomer           = NULL;
  _drop_zones       = NULL;
  _target_drop_zone = NULL;
  _dragging         = FALSE;
  _drag_text        = NULL;
  _zoom_factor      = 1.0;
  _h_adj            = 0.0;
  _v_adj            = 0.0;

  _dnd_config->FetchDataAtEarliest ();
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
    _scrolled_window = GTK_SCROLLED_WINDOW (_glade->GetWidget ("canvas_scrolled_window"));

    _canvas = CreateCanvas ();

    gtk_container_add (GTK_CONTAINER (_scrolled_window), GTK_WIDGET (_canvas));
    gtk_widget_show_all (GTK_WIDGET (_canvas));
  }
}

// --------------------------------------------------------------------------------
void CanvasModule::UnPlug ()
{
  FreezeZoomer ();

  Module::UnPlug ();
}

// --------------------------------------------------------------------------------
void CanvasModule::FreezeZoomer ()
{
  if (_zoomer && _scrolled_window)
  {
    __gcc_extension__ g_signal_handlers_disconnect_by_func (_zoomer,
                                                            (void *) on_zoom_changed, this);
    __gcc_extension__ g_signal_handlers_disconnect_by_func (gtk_scrolled_window_get_hadjustment (_scrolled_window),
                                                            (void *) on_hadjustment_changed, this);
    __gcc_extension__ g_signal_handlers_disconnect_by_func (gtk_scrolled_window_get_vadjustment (_scrolled_window),
                                                            (void *) on_vadjustment_changed, this);
  }
}

// --------------------------------------------------------------------------------
void CanvasModule::OnUnPlugged ()
{
  if (_canvas)
  {
    gtk_widget_destroy (GTK_WIDGET (_canvas));
    _canvas          = NULL;
    _scrolled_window = NULL;
  }
}

// --------------------------------------------------------------------------------
GooCanvas *CanvasModule::CreateCanvas ()
{
  GooCanvas *canvas = GOO_CANVAS (goo_canvas_new ());

  g_object_set (G_OBJECT (canvas),
                "automatic-bounds", TRUE,
                "bounds-padding",   10.0,
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
void CanvasModule::OnZoom (gdouble value)
{
  goo_canvas_set_scale (GetCanvas (),
                        value);
  _zoom_factor = value;
}

// --------------------------------------------------------------------------------
void CanvasModule::SetZoomer (GtkRange *zoomer,
                              gdouble   default_value)
{
  _zoomer      = zoomer;
  _zoom_factor = default_value;
}

// --------------------------------------------------------------------------------
void CanvasModule::RestoreZoomFactor ()
{
  if (_zoomer)
  {
    gtk_range_set_value (GTK_RANGE (_zoomer), _zoom_factor);
    OnZoom (_zoom_factor);

    {
      GtkAdjustment *adj;

      adj = gtk_scrolled_window_get_hadjustment (_scrolled_window);
      gtk_adjustment_set_value (adj,
                                _h_adj);
      adj = gtk_scrolled_window_get_vadjustment (_scrolled_window);
      gtk_adjustment_set_value (adj,
                                _v_adj);
    }

    g_signal_connect (_zoomer,
                      "value-changed",
                      G_CALLBACK (on_zoom_changed), this);
    g_signal_connect (gtk_scrolled_window_get_hadjustment (_scrolled_window),
                      "value-changed",
                      G_CALLBACK (on_hadjustment_changed), this);
    g_signal_connect (gtk_scrolled_window_get_vadjustment (_scrolled_window),
                      "value-changed",
                      G_CALLBACK (on_vadjustment_changed), this);
  }
}

// --------------------------------------------------------------------------------
GooCanvasItem *CanvasModule::GetPlayerImage (GooCanvasItem *parent_item,
                                             const gchar   *common_markup,
                                             Player        *player,
                                             ...)
{
  GooCanvasItem *table_item = goo_canvas_table_new (parent_item,
                                                    NULL);

  if (player)
  {
    GSList *layout_list = NULL;

    if (_filter)
    {
      layout_list = _filter->GetLayoutList ();
    }

    for (guint a = 0; layout_list != NULL; a++)
    {
      Filter::Layout       *attr_layout = (Filter::Layout *) layout_list->data;;
      Attribute            *attr;
      Player::AttributeId  *attr_id;

      if (attr_layout->_desc->_scope == AttributeDesc::LOCAL)
      {
        attr_id = new Player::AttributeId (attr_layout->_desc->_code_name,
                                           GetDataOwner ());
      }
      else
      {
        attr_id = new Player::AttributeId (attr_layout->_desc->_code_name);
      }
      attr = player->GetAttribute (attr_id);
      attr_id->Release ();

      if (attr)
      {
        GdkPixbuf *pixbuf = NULL;

        if (attr_layout->_look == AttributeDesc::GRAPHICAL)
        {
          pixbuf = attr->GetPixbuf ();
        }

        if (pixbuf)
        {
          GooCanvasItem *pix_item = Canvas::PutPixbufInTable (table_item,
                                                              pixbuf,
                                                              0,
                                                              a);
          if (common_markup && (common_markup[0] != 0))
          {
            GRegex     *regex;
            GMatchInfo *match_info;

            regex = g_regex_new ("[0-9]+\\.[0-9]+px",
                                 GRegexCompileFlags (0),
                                 GRegexMatchFlags (0),
                                 NULL);
            g_regex_match (regex,
                           common_markup,
                           GRegexMatchFlags (0),
                           &match_info);

            if (g_match_info_matches (match_info))
            {
              gchar   *word    = g_match_info_fetch (match_info, 0);
              gdouble  width;
              gdouble  height;

              height = g_ascii_strtod (word,
                                       NULL);
              width = height *
                (gdouble) gdk_pixbuf_get_width (attr->GetPixbuf ()) / (gdouble) gdk_pixbuf_get_height (attr->GetPixbuf ());

              g_object_set (G_OBJECT (pix_item),
                            "width",         width,
                            "height",        height,
                            "scale-to-fit",  TRUE,
                            NULL);
              Canvas::SetTableItemAttribute (pix_item,
                                             "right-padding", height/2.0);
              g_free (word);
            }
            g_match_info_free (match_info);
            g_regex_unref (regex);
          }
        }
        else
        {
          gchar    *attr_image = attr->GetUserImage (attr_layout->_look);
          GString  *image;
          gboolean  arg_found = FALSE;

          image = g_string_new    ("<span ");
          image = g_string_append (image,
                                   common_markup);

          {
            va_list  ap;
            gchar   *pango_arg;

            va_start (ap, player);
            for (guint i = 0; (pango_arg = va_arg (ap, char *)); i++)
            {
              if (g_strcmp0 (pango_arg, attr_layout->_desc->_code_name) == 0)
              {
                image = g_string_append (image,
                                         va_arg (ap, char *));
                image = g_string_append (image,
                                         ">");
                arg_found = TRUE;
                break;
              }
            }
            va_end (ap);
          }
          if (arg_found == FALSE)
          {
            image = g_string_append (image,
                                     ">");
          }

          image = g_string_append (image,
                                   attr_image);
          image = g_string_append (image,
                                   "\302\240\302\240");
          image = g_string_append (image,
                                   "</span>");

          {
            GooCanvasItem *item = Canvas::PutTextInTable (table_item,
                                                          image->str,
                                                          0,
                                                          a);
            g_object_set (G_OBJECT (item),
                          "use-markup", TRUE,
                          "ellipsize",  PANGO_ELLIPSIZE_NONE,
                          "wrap",       PANGO_WRAP_CHAR,
                          NULL);
            g_string_free (image,
                           TRUE);
          }

          g_free (attr_image);
        }
      }

      layout_list = g_slist_next (layout_list);
    }
  }

  return table_item;
}

// --------------------------------------------------------------------------------
guint CanvasModule::PreparePrint (GtkPrintOperation *operation,
                                 GtkPrintContext   *context)
{
  return 1;
}

// --------------------------------------------------------------------------------
void CanvasModule::DrawPage (GtkPrintOperation *operation,
                             GtkPrintContext   *context,
                             gint               page_nr)
{
  cairo_t   *cr     = gtk_print_context_get_cairo_context (context);
  GooCanvas *canvas = (GooCanvas *) g_object_get_data (G_OBJECT (operation), "operation_canvas");

  if (canvas == NULL)
  {
    canvas = _canvas;

    Module::DrawPage (operation,
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
                     -canvas_y*scale);
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
gboolean CanvasModule::PreparePreview (GtkPrintOperation        *operation,
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
void CanvasModule::EnableDndOnCanvas ()
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
  gdouble  vvalue  = 0.0;
  gdouble  hvalue  = 0.0;
  GSList  *current = _drop_zones;

  if (_scrolled_window)
  {
    GtkAdjustment *adjustment;

    adjustment = gtk_scrolled_window_get_hadjustment (_scrolled_window);
    hvalue     = gtk_adjustment_get_value (adjustment);

    adjustment = gtk_scrolled_window_get_vadjustment (_scrolled_window);
    vvalue     = gtk_adjustment_get_value (adjustment);
  }

  while (current)
  {
    GooCanvasBounds  bounds;
    DropZone        *drop_zone = (DropZone *) current->data;

    drop_zone->GetBounds (&bounds,
                          _zoom_factor);

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
  if (Module::OnDragMotion (widget,
                            drag_context,
                            x,
                            y,
                            time))
  {
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
        _target_drop_zone = drop_zone;

        return TRUE;
      }
    }
  }

  gdk_drag_status  (drag_context,
                    GDK_ACTION_DEFAULT,
                    time);

  return FALSE;
}

// --------------------------------------------------------------------------------
void CanvasModule::OnDragDataReceived (GtkWidget        *widget,
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
gboolean CanvasModule::OnDragDrop (GtkWidget      *widget,
                                   GdkDragContext *drag_context,
                                   gint            x,
                                   gint            y,
                                   guint           time)
{
  gboolean result = FALSE;

  if (Module::OnDragDrop (widget,
                          drag_context,
                          x,
                          y,
                          time))
  {
    if (_target_drop_zone)
    {
      _target_drop_zone->Unfocus ();

      if (_dnd_config->GetFloatingObject ())
      {
        DropObject (_dnd_config->GetFloatingObject (),
                    NULL,
                    _target_drop_zone);

        result = TRUE;
      }

      _target_drop_zone = NULL;
    }
  }

  return result;
}

// --------------------------------------------------------------------------------
void CanvasModule::OnDragLeave (GtkWidget      *widget,
                                GdkDragContext *drag_context,
                                guint           time)
{
  Module::OnDragLeave (widget,
                       drag_context,
                       time);

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
  Object *drop_object = (Object *) g_object_get_data (G_OBJECT (item),
                                                      "CanvasModule::drop_object");

  if (DragingIsForbidden (drop_object))
  {
    return FALSE;
  }

  if (   (event->button == 1)
      && (event->type == GDK_BUTTON_PRESS))
  {
    _dnd_config->SetFloatingObject (drop_object);

    _drag_x = event->x;
    _drag_y = event->y;

    goo_canvas_convert_from_item_space  (GetCanvas (),
                                         item,
                                         &_drag_x,
                                         &_drag_y);

    {
      GString *string = GetFloatingImage (_dnd_config->GetFloatingObject ());

      _drag_text = goo_canvas_text_new (GetRootItem (),
                                        string->str,
                                        _drag_x,
                                        _drag_y,
                                        -1,
                                        GTK_ANCHOR_NW,
                                        "font", BP_FONT "Bold 14px", NULL);
      g_string_free (string,
                     TRUE);

    }

    SetCursor (GDK_FLEUR);

    _dragging = TRUE;
    _source_drop_zone = drop_zone;
    _target_drop_zone = drop_zone;

    DragObject (_dnd_config->GetFloatingObject (),
                _source_drop_zone);

    drop_zone->Focus ();

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

    _dragging = FALSE;

    goo_canvas_item_remove (_drag_text);
    _drag_text = NULL;

    DropObject (_dnd_config->GetFloatingObject (),
                _source_drop_zone,
                _target_drop_zone);

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

    if (_target_drop_zone)
    {
      _target_drop_zone->Unfocus ();
      _target_drop_zone = NULL;
    }

    {
      DropZone *drop_zone;
      gdouble   adjx = 0.0;
      gdouble   adjy = 0.0;

      if (_scrolled_window)
      {
        GtkAdjustment *hadj = gtk_scrolled_window_get_hadjustment (_scrolled_window);
        GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment (_scrolled_window);

        adjx = gtk_adjustment_get_value (hadj);
        adjy = gtk_adjustment_get_value (vadj);
      }

      drop_zone = GetZoneAt (_drag_x*_zoom_factor - adjx,
                             _drag_y*_zoom_factor - adjy);

      if (DroppingIsAllowed (_dnd_config->GetFloatingObject (),
                             drop_zone))
      {
        drop_zone->Focus ();
        _target_drop_zone = drop_zone;
      }
    }

    SetCursor (GDK_FLEUR);
  }

  return TRUE;
}

// --------------------------------------------------------------------------------
Object *CanvasModule::GetDropObjectFromRef (guint32 ref,
                                            guint   key)
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
gboolean CanvasModule::DragingIsForbidden (Object *object)
{
  return FALSE;
}

// --------------------------------------------------------------------------------
GString *CanvasModule::GetFloatingImage (Object *floating_object)
{
  return g_string_new ("???");
}

// --------------------------------------------------------------------------------
gboolean CanvasModule::DroppingIsAllowed (Object   *floating_object,
                                          DropZone *in_zone)
{
  if (floating_object && in_zone)
  {
    return TRUE;
  }

  return FALSE;
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

// --------------------------------------------------------------------------------
void CanvasModule::on_hadjustment_changed (GtkAdjustment *adjustment,
                                           CanvasModule  *canvas_module)
{
  canvas_module->_h_adj = gtk_adjustment_get_value (adjustment);
}

// --------------------------------------------------------------------------------
void CanvasModule::on_vadjustment_changed (GtkAdjustment *adjustment,
                                           CanvasModule  *canvas_module)
{
  canvas_module->_v_adj = gtk_adjustment_get_value (adjustment);
}

// --------------------------------------------------------------------------------
void CanvasModule::on_zoom_changed (GtkRange     *range,
                                    CanvasModule *canvas_module)
{
  canvas_module->OnZoom (gtk_range_get_value (range));
}
