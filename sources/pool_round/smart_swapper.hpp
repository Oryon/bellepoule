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

  private:
    struct Fencer
    {
      Player   *_player;
      GQuark    _criteria_quark;
      guint     _original_pool;
      guint     _rank;
      gboolean  _is_an_error;

      void Dump (Object *owner);
    };

    class PoolSizes
    {
      public:
        typedef enum
        {
          MIN_SIZE,
          MAX_SIZE,
          END_MARK,

          SIZE_TYPE_LEN
        } SizeType;

        void Configure (guint nb_fencer,
                        guint nb_pool);

        void NewSize (guint old_size,
                      guint new_size);

        guint _available_sizes[SIZE_TYPE_LEN];

      private:
        guint _min_size;
        guint _max_size;
        guint _nb_max;

        guint _nb_max_reached;
    };

    class PoolData
    {
      public:
        Pool       *_pool;
        GHashTable *_criteria_score;
        GList      *_fencer_list;
        guint       _size;
        PoolSizes  *_pool_sizes;

        void AddFencer (Fencer *fencer);

        void RemoveFencer (GList *fencer);

        void ChangeCriteriaScore (GQuark criteria,
                                  gint   delta_score);
    };

    struct CriteriaData
    {
      guint _count;

      guint _max_criteria_occurrence;
      guint _max_floating_fencers;
    };

  private:
    SmartSwapper (Object *rank_attr_id);

    ~SmartSwapper ();

    guint GetPoolIndex (guint fencer_index);

    void ExtractErrors ();

    void ExtractFloatings ();

    Fencer *ExtractFencer (PoolData  *from_pool,
                           GQuark     with_criteria,
                           GSList   **to_list);

    void DispatchErrors ();

    void DispatchFloatings ();

    void DispatchFencers (GSList *list);

    gboolean MoveFencerTo (Fencer       *fencer,
                           PoolData     *data,
                           CriteriaData *criteria_data,
                           guint         max_pool_size);

    static void SetExpectedData (GQuark        quark,
                                 CriteriaData *criteria_data,
                                 SmartSwapper *swapper);

    void InsertCriteria (GHashTable   *table,
                         const GQuark  criteria);

    void CreatePoolTable (GSList *zones);

    void DeletePoolTable ();

    void LookUpDistribution (GSList *fencer_list);

    void StoreSwapping ();

    static gint CompareFencerRank (Fencer *a,
                                   Fencer *b);

  private:
    Object              *_owner;
    guint                _nb_pools;
    PoolData            *_pool_table;
    PoolSizes            _pool_sizes;
    GHashTable          *_criteria_distribution;
    Player::AttributeId *_criteria_id;
    GSList              *_error_list;
    GSList              *_floating_list;
};

#endif
