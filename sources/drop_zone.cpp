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

#include "player.hpp"
#include "match.hpp"
#include "canvas.hpp"

#include "drop_zone.hpp"

// --------------------------------------------------------------------------------
DropZone::DropZone (Module *container)
  : Object ("DropZone")
{
  _container    = container;
  _referee_list = NULL;
  _nb_matchs    = 0;

  Wipe ();
}

// --------------------------------------------------------------------------------
DropZone::~DropZone ()
{
  while (_referee_list)
  {
    RemoveReferee ((Player *) _referee_list->data);
  }
}

// --------------------------------------------------------------------------------
void DropZone::Wipe ()
{
  _back_rect = NULL;
}

// --------------------------------------------------------------------------------
void DropZone::Draw (GooCanvasItem *root_item)
{
  if (_back_rect)
  {
    goo_canvas_item_lower (_back_rect,
                           NULL);
  }
}

// --------------------------------------------------------------------------------
void DropZone::Redraw (gdouble x,
                       gdouble y,
                       gdouble w,
                       gdouble h)
{
  g_object_set (G_OBJECT (_back_rect),
                "x",      x,
                "y",      y,
                "width",  w,
                "height", h,
                NULL);
}

// --------------------------------------------------------------------------------
void DropZone::GetBounds (GooCanvasBounds *bounds,
                          gdouble          zoom_factor)
{
  if (_back_rect)
  {
    goo_canvas_item_get_bounds (_back_rect,
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
  g_object_set (G_OBJECT (_back_rect),
                "fill-color", "Grey",
                NULL);
}

// --------------------------------------------------------------------------------
void DropZone::Unfocus ()
{
  g_object_set (G_OBJECT (_back_rect),
                "fill-color", "White",
                NULL);
}

// --------------------------------------------------------------------------------
void DropZone::AddReferee (Player *referee)
{
  referee->AddMatchs (GetNbMatchs ());
  _container->RefreshMatchRate (referee);

  _referee_list = g_slist_prepend (_referee_list,
                                   referee);
}

// --------------------------------------------------------------------------------
void DropZone::RemoveReferee (Player *referee)
{
  referee->RemoveMatchs (GetNbMatchs ());
  _container->RefreshMatchRate (referee);

  _referee_list = g_slist_remove (_referee_list,
                                  referee);
}

// --------------------------------------------------------------------------------
void DropZone::AddObject (Object *object)
{
}

// --------------------------------------------------------------------------------
void DropZone::RemoveObject (Object *object)
{
}

// --------------------------------------------------------------------------------
guint DropZone::GetNbMatchs ()
{
  return 0;
}
