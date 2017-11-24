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

#include "lasso.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  Lasso::Lasso ()
    : Object ("Lasso")
  {
    _rectangle = NULL;
  }

  // --------------------------------------------------------------------------------
  Lasso::~Lasso ()
  {
  }

  // --------------------------------------------------------------------------------
  void Lasso::GetBounds (GooCanvasBounds *bounds)
  {
    *bounds = _bounds;
  }

  // --------------------------------------------------------------------------------
  void Lasso::Throw (GooCanvasItem  *surface,
                     GdkEventButton *event)
  {
    _x = event->x;
    _y = event->y;

    {
      GooCanvasLineDash *dash = goo_canvas_line_dash_new (2, 3.0, 0.8);

      _rectangle = goo_canvas_rect_new (surface,
                                        _x, _y,
                                        0.0, 0.0,
                                        "line-width",   1.0,
                                        "line-dash",    dash,
                                        NULL);
      goo_canvas_line_dash_unref (dash);
    }

    goo_canvas_item_get_bounds (_rectangle,
                                &_bounds);
  }

  // --------------------------------------------------------------------------------
  void Lasso::Pull ()
  {
    if (_rectangle)
    {
      goo_canvas_item_remove (_rectangle);
      _rectangle = NULL;
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Lasso::OnCursorMotion (GdkEventMotion *event)
  {
    if (_rectangle)
    {
      gdouble top;
      gdouble left;
      gdouble width;
      gdouble height;

      if (event->x < _x)
      {
        width = _x - event->x;
        left  = event->x;
      }
      else
      {
        width = event->x - _x;
        left  = _x;
      }

      if (event->y < _y)
      {
        height = _y - event->y;
        top    = event->y;
      }
      else
      {
        height = event->y - _y;
        top    = _y;
      }

      g_object_set (G_OBJECT (_rectangle),
                    "x",      left,
                    "y",      top,
                    "width",  width,
                    "height", height,
                    NULL);
      goo_canvas_item_get_bounds (_rectangle,
                                  &_bounds);
      return TRUE;
    }

    return FALSE;
  }
}
