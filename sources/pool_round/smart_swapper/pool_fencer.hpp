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

#ifndef pool_fencer_hpp
#define pool_fencer_hpp

#include "util/object.hpp"
#include "common/player.hpp"

namespace SmartSwapper
{
  class PoolData;

  class Fencer : public Object
  {
    public:
      Player   *_player;
      GQuark   *_criteria_quarks;
      PoolData *_original_pool;
      PoolData *_new_pool;
      guint     _rank;

      Fencer (Player   *player,
              guint     rank,
              PoolData *original_pool,
              guint     criteria_count);

      static gint CompareRank (Fencer *a,
                               Fencer *b);

    private:
      ~Fencer ();
  };
}

#endif
