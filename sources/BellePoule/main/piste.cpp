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

#include "piste.hpp"

Piste *Piste::_selected = NULL;

// --------------------------------------------------------------------------------
Piste::Piste (GooCanvasItem *parent,
              guint          id)
  : Object ("Piste")
{
  _id         = id;
  _dragging   = FALSE;
  _horizontal = TRUE;

  _root = goo_canvas_group_new (parent,
                                NULL);

  {
    _rect = goo_canvas_rect_new (_root,
                                 0.0, 0.0, 160.0,  20.0,
                                 "line-width",   0.5,
                                 "radius-x",     1.0,
                                 "radius-y",     1.0,
                                 "stroke-color", "black",
                                 "fill-color",   "lightblue",
                                 NULL);

    g_signal_connect (_rect,
                      "focus_in_event",
                      G_CALLBACK (OnFocusIn),
                      this);

    g_signal_connect (_rect,
                      "focus_out_event",
                      G_CALLBACK (OnFocusOut),
                      this);

    g_signal_connect (_rect,
                      "motion_notify_event",
                      G_CALLBACK (OnMotionNotify),
                      this);

    g_signal_connect (_rect,
                      "button_press_event",
                      G_CALLBACK (OnPisteSelected),
                      this);

    g_signal_connect (_rect,
                      "button_release_event",
                      G_CALLBACK (OnPisteUnSelected),
                      this);
  }

  {
    gchar *name = g_strdup_printf ("%d", _id);

    goo_canvas_text_new (_root,
                         name,
                         1.0, 1.0,
                         -1.0,
                         GTK_ANCHOR_NW,
                         "fill-color", "black",
                         "font", BP_FONT "bold 10px",
                         NULL);
    g_free (name);
  }

  goo_canvas_item_translate (_root, 5.0, 5.0);
}

// --------------------------------------------------------------------------------
Piste::~Piste ()
{
  if (_selected == this)
  {
    _selected = NULL;
  }

  goo_canvas_item_remove (_root);
}

// --------------------------------------------------------------------------------
void Piste::Translate (gdouble tx,
                       gdouble ty)
{
  goo_canvas_item_translate (_root,
                             tx,
                             ty);
}

// --------------------------------------------------------------------------------
void Piste::Rotate ()
{
  if (_horizontal)
  {
    goo_canvas_item_rotate (_root,
                            90.0,
                            0.0,
                            0.0);
    _horizontal = FALSE;
  }
  else
  {
    goo_canvas_item_rotate (_root,
                            -90.0,
                            0.0,
                            0.0);
    _horizontal = TRUE;
  }
}

// --------------------------------------------------------------------------------
Piste *Piste::GetSelected ()
{
  return _selected;
}

// --------------------------------------------------------------------------------
gboolean Piste::OnPisteSelected (GooCanvasItem  *item,
                                 GooCanvasItem  *target,
                                 GdkEventButton *event,
                                 Piste          *piste)
{
  if (event->button == 1)
  {
    piste->_dragging = TRUE;
    piste->_drag_x = event->x;
    piste->_drag_y = event->y;

    goo_canvas_grab_focus (goo_canvas_item_get_canvas (item),
                           item);
    return TRUE;
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean Piste::OnPisteUnSelected (GooCanvasItem  *item,
                                   GooCanvasItem  *target,
                                   GdkEventButton *event,
                                   Piste          *piste)
{
  if (event->button == 1)
  {
    piste->_dragging = FALSE;

    return TRUE;
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean Piste::OnFocusIn (GooCanvasItem *goo_rect,
                           GooCanvasItem *target,
                           GdkEventFocus *event,
                           Piste         *piste)
{
  _selected = piste;

  g_object_set (piste->_rect,
                "line-width", 1.0,
                NULL);
  return TRUE;
}

// --------------------------------------------------------------------------------
gboolean Piste::OnFocusOut (GooCanvasItem *goo_rect,
                            GooCanvasItem *target,
                            GdkEventFocus *event,
                            Piste         *piste)
{
  _selected = NULL;

  g_object_set (piste->_rect,
                "line-width", 0.5,
                NULL);
  return TRUE;
}

// --------------------------------------------------------------------------------
gboolean Piste::OnMotionNotify (GooCanvasItem  *item,
                                GooCanvasItem  *target,
                                GdkEventMotion *event,
                                Piste          *piste)
{
  if (piste->_dragging && (event->state & GDK_BUTTON1_MASK))
  {
    double new_x = event->x;
    double new_y = event->y;

    piste->Translate (new_x - piste->_drag_x,
                      new_y - piste->_drag_y);
  }

  return TRUE;
}
