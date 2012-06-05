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

#include "object.hpp"
#include "player.hpp"

#include "swapper.hpp"

class SmartSwapper : public Object, public Swapper
{
  public:
    static Swapper *Create (Object *owner);

    void Delete ();

    void Swap (GSList *zones,
               gchar  *criteria,
               GSList *fencer_list);

    Player *GetNextPlayer (Pool *for_pool);

  private:
    struct Fencer
    {
      Player *_player;
      GQuark  _criteria_quark;
      guint   _original_pool;

      void Dump (Object *owner);
    };

    struct PoolData
    {
      Pool       *_pool;
      GHashTable *_criteria_score;
      GList      *_fencer_list;
    };

    struct CriteriaData
    {
      guint _count;

      guint _max_criteria_occurrence;
      guint _max_floating_fencers;
      guint _floating_fencers;
      guint _has_errors;
    };

  private:
    SmartSwapper (Object *rank_attr_id);

    ~SmartSwapper ();

    guint GetPoolIndex (guint fencer_index);

    void ExtractErrors ();

    void ExtractFloatings ();

    void ExtractFencers (PoolData  *from_pool,
                         GQuark     with_criteria,
                         guint      number,
                         GSList   **to_list);

    void DispatchErrors ();

    void DispatchFloatings ();

    void DispatchFencers (GSList   *list,
                          gboolean  try_original_pool_first);

    gboolean FencerCanGoTo (Fencer       *fencer,
                            PoolData     *data,
                            CriteriaData *criteria_data,
                            guint         goal);

    static void SetExpectedData (GQuark        quark,
                                 CriteriaData *criteria_data,
                                 SmartSwapper *swapper);

    void InsertCriteria (GHashTable   *table,
                         const GQuark  criteria);

    void CreatePoolTable (GSList *zones);

    void DeletePoolTable ();

    void LookUpDistribution (GSList *fencer_list);

    void StoreSwapping ();

  private:
    Object              *_owner;
    guint                _nb_pools;
    PoolData            *_pool_table;
    guint                _max_pool_size;
    guint                _min_pool_size;
    GHashTable          *_criteria_distribution;
    Player::AttributeId *_criteria_id;
    GSList              *_error_list;
    GSList              *_floating_list;
};

#endif
