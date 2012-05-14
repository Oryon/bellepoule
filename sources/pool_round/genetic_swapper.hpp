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

#ifndef genetic_swapper_hpp
#define genetic_swapper_hpp

#include <ga/GAListGenome.h>

#include "swapper.hpp"

class GAGenome;
class Player;

class GeneticSwapper : public Object, public Swapper
{
  public:
    static Swapper *Create (GSList *pools,
                            gchar  *criteria,
                            GSList *fencer_list,
                            Object *owner);

    void Delete ();

    Player *GetNextPlayer (Pool *for_pool);

  private:
    GeneticSwapper (GSList *pools,
                    gchar  *criteria,
                    GSList *fencer_list,
                    Object *rank_attr_id);

    ~GeneticSwapper ();

    guint GetPoolIndex (guint fencer_index);

  private:
    struct Gene
    {
      Player *_fencer;
      GQuark  _criteria_quark;
      guint   _original_pool;
      guint   _current_pool;

      bool operator == (const Gene &f);
      bool operator != (const Gene &f);

      void Dump (Object *owner);
    };

    struct PoolData
    {
      Pool       *_pool;
      GHashTable *_criteria_count;
    };

    struct CriteriaData
    {
      guint _count;

      guint _ideal_max_fencer1;
      guint _ideal_max_pool1;

      guint _ideal_max_fencer2;
      guint _ideal_max_pool2;

      guint _real_max_pool1;
      guint _real_max_pool2;
    };

    typedef GAListGenome<Gene *> Genome;

    static const guint SEED        = 33;
    static const guint GENERATIONS = 100;

    Object     *_owner;
    GRand      *_rand;
    guint       _nb_pools;
    PoolData   *_pool_table;
    guint       _nb_gene;
    GSList     *_gene_list;
    GSList     *_top_fencer_quark;
    Genome     *_initial_genome;
    GHashTable *_criteria_distribution;

    Player::AttributeId *_criteria_id;

    static float Objective (GAGenome &g);

    static void  Initializer (GAGenome &g);

    static GABoolean Terminator (GAGeneticAlgorithm &ga);

    static void SetIdealData (GQuark          key,
                              CriteriaData   *data,
                              GeneticSwapper *swapper);

    static void ResetRealData (GQuark          key,
                               CriteriaData   *data,
                               GeneticSwapper *swapper);

    void InsertCriteria (GHashTable   *table,
                         const GQuark  criteria);

};

#endif
