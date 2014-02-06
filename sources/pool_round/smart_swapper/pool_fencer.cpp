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

#include "util/object.hpp"

#include "pool_round/pool.hpp"
#include "pool_round/pool_zone.hpp"

#include "pool_data.hpp"
#include "pool_fencer.hpp"

namespace SmartSwapper
{
  // --------------------------------------------------------------------------------
  Fencer::Fencer (Player   *player,
                  guint     rank,
                  PoolData *original_pool,
                  guint     criteria_count)
    : Object ("SmartSwapper::Fencer")
  {
    _new_pool        = NULL;
    _player          = player;
    _rank            = rank;
    _original_pool   = original_pool;
    _criteria_quarks = g_new0 (GQuark, criteria_count);
  }

  // --------------------------------------------------------------------------------
  Fencer::~Fencer ()
  {
    g_free (_criteria_quarks);
  }
}
