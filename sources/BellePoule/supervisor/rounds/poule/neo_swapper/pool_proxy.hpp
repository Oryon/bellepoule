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

#pragma once

#include "util/object.hpp"

class Player;

namespace Pool
{
  class Pool;
}

namespace NeoSwapper
{
  class FencerProxy;
  class Criteria;

  class PoolProxy : public Object
  {
    public:
      Pool::Pool *_pool;
      GList      *_fencer_list;
      guint       _id;
      guint       _size;

    public:
      PoolProxy (Pool::Pool *pool,
                 guint       id,
                 guint       criteria_count);

      FencerProxy *GetFencerProxy (Player *fencer);

      void AddFencer (FencerProxy *fencer);

      void InsertFencer (FencerProxy *fencer);

      void RemoveFencer (FencerProxy *fencer);

      GList *GetFencers (guint  from,
                         guint  criteria_depth,
                         GQuark criteria_quark);

      FencerProxy *GetFencer (guint  at,
                              guint  criteria_depth,
                              GQuark criteria_quark);

    private:
      guint _criteria_count;

      ~PoolProxy ();
  };
}
