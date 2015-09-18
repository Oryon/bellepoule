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

#include "util/canvas.hpp"
#include "util/player.hpp"
#include "../../match.hpp"

#include "pool_zone.hpp"

namespace Pool
{
  // --------------------------------------------------------------------------------
  PoolZone::PoolZone (Module *container,
                      Pool   *pool)
    : RefereeZone (container)
  {
    _pool = pool;
  }

  // --------------------------------------------------------------------------------
  PoolZone::~PoolZone ()
  {
    while (_referee_list)
    {
      RemoveReferee ((Player *) _referee_list->data);
    }
  }

  // --------------------------------------------------------------------------------
  Pool *PoolZone::GetPool ()
  {
    return _pool;
  }

  // --------------------------------------------------------------------------------
  void PoolZone::Draw (GooCanvasItem *root_item)
  {
    if (_drop_rect == NULL)
    {
      _drop_rect = goo_canvas_rect_new (root_item,
                                        0, 0,
                                        0, 0,
                                        "stroke-pattern", NULL,
                                        NULL);
      RefereeZone::Draw (root_item);
    }
  }

  // --------------------------------------------------------------------------------
  void PoolZone::AddReferee (Player *referee)
  {
    _pool->AddReferee (referee);
    RefereeZone::AddReferee (referee);
  }

  // --------------------------------------------------------------------------------
  void PoolZone::RemoveReferee (Player *referee)
  {
    _pool->RemoveReferee (referee);
    RefereeZone::RemoveReferee (referee);
  }

  // --------------------------------------------------------------------------------
  guint PoolZone::GetNbMatchs ()
  {
    return _pool->GetNbMatchs ();
  }
}
