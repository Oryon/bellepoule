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
#include "table.hpp"
#include "canvas.hpp"

#include "pool_zone.hpp"

// --------------------------------------------------------------------------------
PoolZone::PoolZone (Module *container,
                    Pool   *pool)
: DropZone (container)
{
  _pool = pool;
}

// --------------------------------------------------------------------------------
PoolZone::~PoolZone ()
{
}

// --------------------------------------------------------------------------------
Pool *PoolZone::GetPool ()
{
  return _pool;
}

// --------------------------------------------------------------------------------
void PoolZone::Draw (GooCanvasItem *root_item)
{
  if (_back_rect == NULL)
  {
    _back_rect = goo_canvas_rect_new (root_item,
                                      0, 0,
                                      0, 0,
                                      "pointer-events", GOO_CANVAS_EVENTS_VISIBLE,
                                      "stroke-pattern", NULL,
                                      NULL);
    DropZone::Draw (root_item);
  }
}

// --------------------------------------------------------------------------------
void PoolZone::AddReferee (Player *referee)
{
  _pool->AddReferee (referee);
  DropZone::AddReferee (referee);
}

// --------------------------------------------------------------------------------
void PoolZone::RemoveReferee (Player *referee)
{
  _pool->RemoveReferee (referee);
  DropZone::RemoveReferee (referee);
}

// --------------------------------------------------------------------------------
guint PoolZone::GetNbMatchs ()
{
  guint   result  = 0;

  return result;
}
