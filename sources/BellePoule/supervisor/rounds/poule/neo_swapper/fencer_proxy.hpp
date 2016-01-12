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

#ifndef fencer_proxy_hpp
#define fencer_proxy_hpp

#include "util/object.hpp"
#include "util/player.hpp"

namespace NeoSwapper
{
  class PoolProxy;
  class Criteria;

  class FencerProxy : public Object
  {
    public:
      Player    *_player;
      PoolProxy *_original_pool;
      PoolProxy *_new_pool;
      guint      _rank;

      FencerProxy (Player    *player,
                   guint      rank,
                   PoolProxy *original_pool,
                   guint      criteria_count);

      gboolean HasQuark (guint  at_depth,
                         GQuark quark);

      void SetCriteria (guint     at_depth,
                        Criteria *criteria);

      Criteria *GetCriteria (guint at_depth);

      static gint CompareRank (FencerProxy *a,
                               FencerProxy *b);

    private:
      Criteria **_criterias;

      ~FencerProxy ();
  };
}

#endif
