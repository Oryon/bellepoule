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

#include <math.h>

#include "util/module.hpp"

#include "piste.hpp"

// --------------------------------------------------------------------------------
Piste::Piste (GooCanvasItem *parent,
              Listener      *listener)
  : Object ("Piste")
{
  _horizontal = TRUE;
  _listener   = listener;

  _root_item = goo_canvas_group_new (parent,
                                     NULL);

  {
    _rect_item = goo_canvas_rect_new (_root_item,
                                      0.0, 0.0,
                                      _W, _H,
                                      "line-width",   _BORDER_W,
                                      "stroke-color", "black",
                                      "fill-color",   "lightblue",
                                      NULL);
    MonitorEvent (_rect_item);
  }

  {
    _color_item = goo_canvas_rect_new (_root_item,
                                       _BORDER_W, _BORDER_W,
                                       _W/10.0, _H - _BORDER_W*2.0,
                                       "line-width", 0.0,
                                       "fill-color", "darkgreen",
                                       NULL);
    MonitorEvent (_color_item);
  }

  {
    _progress_item = goo_canvas_rect_new (_root_item,
                                          _W/10.0 + _BORDER_W,
                                          _H - (_H/10.0),
                                          _W/2.0,
                                          (_H/10.0)-_BORDER_W,
                                          "fill-color", "darkgreen",
                                          "line-width", 0.0,
                                          NULL);
    MonitorEvent (_progress_item);
  }

  {
    _id_item = goo_canvas_text_new (_root_item,
                                    "",
                                    1.0, 1.0,
                                    -1.0,
                                    GTK_ANCHOR_NW,
                                    "fill-color", "black",
                                    "font", BP_FONT "bold 10px",
                                    NULL);
    MonitorEvent (_id_item);
  }

  {
    _title_item = goo_canvas_text_new (_root_item,
                                       "Sabre-Homme-Cadet",
                                       _W/2.0, 1.0,
                                       -1.0,
                                       GTK_ANCHOR_NORTH,
                                       "fill-color", "black",
                                       "font", BP_FONT "bold 8px",
                                       NULL);
    MonitorEvent (_title_item);
  }

  {
    _match_item = goo_canvas_text_new (_root_item,
                                       "Poule N°1",
                                       _W/2.0, 9.0,
                                       -1.0,
                                       GTK_ANCHOR_NORTH,
                                       "fill-color", "black",
                                       "font", BP_FONT "bold 8px",
                                       NULL);
    MonitorEvent (_match_item);
  }

  {
    Module    *owner  = dynamic_cast <Module *> (listener);
    GdkPixbuf *pixbuf = owner->GetPixbuf (GTK_STOCK_DIALOG_AUTHENTICATION);

    _status_item = goo_canvas_image_new (_root_item,
                                         pixbuf,
                                         (_W - 10.0) * 2.0,
                                         1.0,
                                         NULL);
    MonitorEvent (_status_item);

    goo_canvas_item_scale (_status_item,
                           0.5,
                           0.5);

    g_object_unref (pixbuf);
  }

  goo_canvas_item_translate (_root_item,
                             _RESOLUTION,
                             _RESOLUTION);

  SetId (1);
}

// --------------------------------------------------------------------------------
Piste::~Piste ()
{
  goo_canvas_item_remove (_root_item);
}

// --------------------------------------------------------------------------------
void Piste::Translate (gdouble tx,
                       gdouble ty)
{
  if (_horizontal)
  {
    goo_canvas_item_translate (_root_item,
                               tx,
                               ty);
  }
  else
  {
    goo_canvas_item_translate (_root_item,
                               ty,
                               -tx);
  }
}

// --------------------------------------------------------------------------------
void Piste::Rotate ()
{
  if (_horizontal)
  {
    goo_canvas_item_rotate (_root_item,
                            90.0,
                            0.0,
                            0.0);
    _horizontal = FALSE;
  }
  else
  {
    goo_canvas_item_rotate (_root_item,
                            -90.0,
                            0.0,
                            0.0);
    _horizontal = TRUE;
  }
}

// --------------------------------------------------------------------------------
guint Piste::GetId ()
{
  return _id;
}

// --------------------------------------------------------------------------------
void Piste::SetId (guint id)
{
  _id = id;

  {
    gchar *name = g_strdup_printf ("%d", _id);

    g_object_set (_id_item,
                  "text", name,
                  NULL);

    g_free (name);
  }
}

// --------------------------------------------------------------------------------
gboolean Piste::OnButtonPress (GooCanvasItem  *item,
                               GooCanvasItem  *target,
                               GdkEventButton *event,
                               Piste          *piste)
{
  if (event->button == 1)
  {
    piste->_listener->OnPisteButtonEvent (piste,
                                          event);

    return TRUE;
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean Piste::OnButtonRelease (GooCanvasItem  *item,
                                 GooCanvasItem  *target,
                                 GdkEventButton *event,
                                 Piste          *piste)
{
  if (event->button == 1)
  {
    piste->_listener->OnPisteButtonEvent (piste,
                                          event);

    return TRUE;
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
void Piste::Select ()
{
  g_object_set (_rect_item,
                "line-width", _BORDER_W*2.0,
                NULL);
}

// --------------------------------------------------------------------------------
void Piste::UnSelect ()
{
  g_object_set (_rect_item,
                "line-width", _BORDER_W,
                NULL);
}

// --------------------------------------------------------------------------------
gboolean Piste::OnMotionNotify (GooCanvasItem  *item,
                                GooCanvasItem  *target,
                                GdkEventMotion *event,
                                Piste          *piste)
{
  piste->_listener->OnPisteMotionEvent (piste,
                                        event);

  return TRUE;
}

// --------------------------------------------------------------------------------
void Piste::GetHorizontalCoord (gdouble *x,
                                gdouble *y)
{
  if (_horizontal == FALSE)
  {
    gdouble swap = *x;

    *x = -(*y);
    *y = swap;
  }
}

// --------------------------------------------------------------------------------
gdouble Piste::GetGridAdjustment (gdouble coordinate)
{
  gdouble modulo = fmod (coordinate, _RESOLUTION);

  if (modulo > _RESOLUTION/2.0)
  {
    return _RESOLUTION - modulo;
  }
  else
  {
    return -modulo;
  }
}

// --------------------------------------------------------------------------------
void Piste::AlignOnGrid ()
{
  GooCanvasBounds bounds;

  goo_canvas_item_get_bounds (_root_item,
                              &bounds);

  goo_canvas_item_translate (_root_item,
                             GetGridAdjustment (bounds.x1),
                             GetGridAdjustment (bounds.y1));
}

// --------------------------------------------------------------------------------
void Piste::MonitorEvent (GooCanvasItem *item)
{
  g_signal_connect (item,
                    "motion_notify_event",
                    G_CALLBACK (OnMotionNotify),
                    this);

  g_signal_connect (item,
                    "button_press_event",
                    G_CALLBACK (OnButtonPress),
                    this);

  g_signal_connect (item,
                    "button_release_event",
                    G_CALLBACK (OnButtonRelease),
                    this);
}