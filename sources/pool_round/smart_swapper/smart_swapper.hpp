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

#ifndef smart_swapper_hpp
#define smart_swapper_hpp

#include "util/object.hpp"
#include "common/player.hpp"
#include "pool_round/swapper.hpp"

#include "criteria_profile.hpp"
#include "pool_fencer.hpp"
#include "pool_sizes.hpp"
#include "pool_data.hpp"

namespace SmartSwapper
{
  class SmartSwapper : public Object, public Pool::Swapper
  {
    public:
      static Pool::Swapper *Create (Object *owner);

      void Delete ();

      void Init (GSList *zones,
                 guint   fencer_count);

      void Swap (GSList *criteria_list,
                 GSList *fencer_list);

      guint HasErrors ();

      guint GetOverCount ();

      guint GetMoved ();

    private:
      SmartSwapper (Object *rank_attr_id);

      virtual ~SmartSwapper ();

      guint GetPoolIndex (guint fencer_index);

      void Iterate ();

      void ExtractOverPopulationErrors ();

      void FindLackOfPopulationErrors ();

      void ExtractFloatings ();

      Fencer *ExtractFencer (PoolData  *from_pool,
                             GQuark     with_criteria,
                             GList    **to_list);

      void DispatchErrors ();

      void DispatchFloatings ();

      void DispatchFencers (GList *list);

      gboolean MoveFencerTo (Fencer   *fencer,
                             PoolData *pool_data,
                             guint     max_pool_size);

      static void SetExpectedData (GQuark           quark,
                                   CriteriaProfile *criteria_data,
                                   SmartSwapper    *swapper);

      void InsertCriteria (Player              *player,
                           GHashTable          *criteria_distribution,
                           Player::AttributeId *criteria_id);

      void CreatePoolTable (GSList *zones);

      void DeletePoolTable ();

      void AddPlayerToPool (Player   *player,
                            PoolData *pool_data,
                            GSList   *criteria_list);

      GHashTable *LookUpDistribution (GSList              *fencer_list,
                                      guint                criteria_index,
                                      Player::AttributeId *criteria_id);

      void StoreSwapping ();

      static gint CompareFencerRank (Fencer *a,
                                     Fencer *b);

      PoolData *GetPoolToTry (guint index);

      void DumpPools ();

    private:
      Object      *_owner;
      GSList      *_zones;
      guint        _nb_pools;
      PoolData   **_pool_table;
      PoolSizes    _pool_sizes;
      GHashTable **_distributions;
      GList       *_error_list;
      GList       *_floating_list;
      GHashTable  *_lack_table;
      GSList      *_remaining_errors;
      guint        _first_pool_to_try;
      gboolean     _has_errors;
      guint        _over_count;
      guint        _moved;
      guint        _criteria_count;

      guint       _current_criteria_index;
      GQuark      _previous_criteria_quark;
  };
}

#endif
