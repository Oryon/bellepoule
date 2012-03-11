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

    static guint GetPoolIndex (guint fencer_index);

  private:
    struct Fencer
    {
      Player *_fencer;
      GQuark  _criteria_value;
      guint   _original_pool;
      guint   _current_pool;

      bool operator == (Fencer const &f);
      bool operator != (Fencer const &f);

      void Dump ();
    };

    struct PoolData
    {
      Pool   *_pool;
      guint   _size;
      guint   _error;
    };

    typedef GAListGenome<Fencer *> Genome;

    static const guint _seed = 33;

    static GRand    *_rand;
    static guint     _nb_pools;
    static PoolData *_pool_table;
    static guint     _nb_fencers;
    static GSList   *_fencer_list;
    static Genome   *_initial_genome;

    Player::AttributeId *_criteria_id;

    static float Objective (GAGenome &g);

    static void  Initializer (GAGenome &g);

    static int CrossOver (const GAGenome &g1,
                          const GAGenome &g2,
                          GAGenome       *c1,
                          GAGenome       *c2);

    static float Comparator (const GAGenome &g1,
                             const GAGenome &g2);
};

#endif
