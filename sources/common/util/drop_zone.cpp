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

#include "canvas.hpp"

#include "drop_zone.hpp"

// --------------------------------------------------------------------------------
DropZone::DropZone (Module *container)
  : Object ("DropZone")
{
  _container = container;

  Wipe ();
}

// --------------------------------------------------------------------------------
DropZone::~DropZone ()
{
}

// --------------------------------------------------------------------------------
void DropZone::Wipe ()
{
  _drop_rect = NULL;
}

// --------------------------------------------------------------------------------
void DropZone::Draw (GooCanvasItem *root_item)
{
  if (_drop_rect)
  {
    goo_canvas_item_lower (_drop_rect,
                           NULL);
  }
}

// --------------------------------------------------------------------------------
void DropZone::Redraw (gdouble x,
                       gdouble y,
                       gdouble w,
                       gdouble h)
{
  g_object_set (G_OBJECT (_drop_rect),
                "x",        x,
                "y",        y,
                "width",    w,
                "height",   h,
                NULL);
}

// --------------------------------------------------------------------------------
void DropZone::GetBounds (GooCanvasBounds *bounds,
                          gdouble          zoom_factor)
{
  if (_drop_rect)
  {
    goo_canvas_item_get_bounds (_drop_rect,
                                bounds);
    bounds->x1 *= zoom_factor;
    bounds->x2 *= zoom_factor;
    bounds->y1 *= zoom_factor;
    bounds->y2 *= zoom_factor;
  }
  else
  {
    bounds->x1 = 0;
    bounds->x2 = 0;
    bounds->y1 = 0;
    bounds->y2 = 0;
  }
}

// --------------------------------------------------------------------------------
void DropZone::Focus ()
{
  g_object_set (G_OBJECT (_drop_rect),
                "fill-color", "Grey",
                NULL);
}

// --------------------------------------------------------------------------------
void DropZone::Unfocus ()
{
  g_object_set (G_OBJECT (_drop_rect),
                "fill-color", "White",
                NULL);
}

// --------------------------------------------------------------------------------
void DropZone::AddObject (Object *object)
{
}

// --------------------------------------------------------------------------------
void DropZone::RemoveObject (Object *object)
{
}
