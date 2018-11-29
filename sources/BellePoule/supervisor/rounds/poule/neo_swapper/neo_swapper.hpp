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
#include "util/player.hpp"

#include "../swapper.hpp"

namespace NeoSwapper
{
  class PoolProxy;
  class FencerProxy;

  class Swapper : public Object, public Pool::Swapper
  {
    public:
      static Pool::Swapper *Create (Object *owner);

    private:
      void Delete () override;

      void Configure (GSList *zones,
                      GSList *criteria_list) override;

      void Swap (GSList *fencer_list) override;

      void MoveFencer (Player         *player,
                       Pool::PoolZone *from_pool_zone,
                       Pool::PoolZone *to_pool_zone) override;

      void CheckCurrentDistribution () override;

      guint HasErrors () override;

      gboolean IsAnError (Player *fencer) override;

      guint GetMoved () override;

    private:
      Swapper (Object *rank_attr_id);

      ~Swapper () override;

      void Clean ();

      void StoreSwapping ();

      void DumpPools ();

    private:
      void CreatePoolTable (GSList *zones);

      void DeletePoolTable ();

      void InjectFencer (Player *fencer,
                         guint   pool_id) override;

      FencerProxy *ManageFencer (Player    *player,
                                 PoolProxy *pool_proxy);

      void InsertCriteria (FencerProxy         *fencer,
                           GHashTable          *criteria_distribution,
                           Player::AttributeId *criteria_id,
                           guint                depth);

    private:
      void FindErrors (guint depth);

      void DispatchErrors (guint depth);

      gboolean DispatchFencer (FencerProxy *fencer,
                               guint        depth,
                               gboolean     force = FALSE);

      gboolean FencerCanGoTo (FencerProxy *fencer,
                              PoolProxy   *pool_proxy,
                              guint        depth);

    private:
      Object      *_owner;
      GSList      *_zones;
      GList       *_fencer_list;
      guint        _nb_pools;
      PoolProxy  **_pool_table;
      GHashTable **_distributions;
      GList       *_error_list;
      GSList      *_criteria_list;
      guint        _criteria_count;
  };
}
