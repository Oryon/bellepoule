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

#ifndef neo_swapper_hpp
#define neo_swapper_hpp

#include "util/object.hpp"
#include "util/player.hpp"
#include "../swapper.hpp"

#include "criteria.hpp"
#include "fencer_proxy.hpp"
#include "pool_proxy.hpp"
#include "list_crawler.hpp"

namespace NeoSwapper
{
  class Swapper : public Object, public Pool::Swapper
  {
    public:
      static Pool::Swapper *Create (Object *owner);

    private:
      void Delete ();

      void Configure (GSList *zones,
                      GSList *criteria_list);

      void Swap (GSList *fencer_list);

      void MoveFencer (Player         *player,
                       Pool::PoolZone *from_pool_zone,
                       Pool::PoolZone *to_pool_zone);

      void CheckCurrentDistribution ();

      guint HasErrors ();

      gboolean IsAnError (Player *fencer);

      guint GetMoved ();

    private:
      Swapper (Object *rank_attr_id);

      virtual ~Swapper ();

      void Clean ();

      void StoreSwapping ();

      void DumpPools ();

    private:
      void CreatePoolTable (GSList *zones);

      void DeletePoolTable ();

      void ManageFencer (Player    *player,
                         PoolProxy *pool_proxy);

      void InsertCriteria (FencerProxy         *fencer,
                           GHashTable          *criteria_distribution,
                           Player::AttributeId *criteria_id,
                           guint                depth);

    private:
      void FindErrors (guint deep);

      void DispatchErrors (guint deep);

      GList *DispatchFencers (GList *list,
                              guint  deep);

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

#endif
