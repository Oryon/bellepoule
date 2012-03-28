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

#include <iostream>
#include <ga/GASStateGA.h>
#include <ga/GASimpleGA.h>
#include <ga/GAListGenome.h>
#include <ga/garandom.h>
#include <ga/std_stream.h>

#include "object.hpp"
#include "pool.hpp"

#include "genetic_swapper.hpp"

#define cout STD_COUT

// --------------------------------------------------------------------------------
void GeneticSwapper::Gene::Dump (Object *owner)
{
  Player::AttributeId  previous_rank_attr_id ("previous_stage_rank", owner);
  Attribute *previous_stage_rank = _fencer->GetAttribute (&previous_rank_attr_id);

  g_print ("%d %20ss >> %s\n", previous_stage_rank->GetUIntValue (), _fencer->GetName (), g_quark_to_string (_criteria_quark));
}

// --------------------------------------------------------------------------------
bool GeneticSwapper::Gene::operator == (const Gene &f)
{
  return (_fencer == f._fencer);
}

// --------------------------------------------------------------------------------
bool GeneticSwapper::Gene::operator != (const Gene &f)
{
  return (_fencer != f._fencer);
}

// --------------------------------------------------------------------------------
GeneticSwapper::GeneticSwapper (GSList *pools,
                                gchar  *criteria,
                                GSList *fencer_list,
                                Object *owner)
: Object ("GeneticSwapper")
{
  Genome genome (Objective, this);

  _owner = owner;
  _rand  = g_rand_new_with_seed (SEED);

  {
    _nb_pools   = g_slist_length (pools);
    _pool_table = g_new (PoolData,
                         _nb_pools);

    {
      GSList *current = pools;

      for (guint i = 0; i < _nb_pools; i++)
      {
        _pool_table[i]._pool           = (Pool *) current->data;
        _pool_table[i]._criteria_count = NULL;

        current = g_slist_next (current);
      }
    }
  }

  _initial_genome = new Genome ();
  _gene_list      = NULL;
  _nb_gene        = 0;

  _criteria_distribution = g_hash_table_new_full (NULL,
                                                  NULL,
                                                  NULL,
                                                  g_free);
  _criteria_id = new Player::AttributeId (criteria);

  {
    GSList *current = fencer_list;

    _top_fencer_quark = NULL;
    for (guint i = 0; current != NULL; i++)
    {
      Player *fencer = (Player *) current->data;
      GQuark  quark  = 0;

      // criteria
      {
        Attribute *criteria_attr = fencer->GetAttribute (_criteria_id);

        if (criteria_attr)
        {
          quark = g_quark_from_string (criteria_attr->GetUserImage ());

          InsertCriteria (_criteria_distribution,
                          quark);
          _top_fencer_quark = g_slist_prepend (_top_fencer_quark,
                                               (void *) quark);
        }
      }

      // Prevent the pool leader fencers from being moved
      if (i >= _nb_pools)
      {
        Gene *gene = new Gene;

        gene->_fencer         = fencer;
        gene->_criteria_quark = quark;

        _nb_gene++;
        _gene_list = g_slist_append (_gene_list,
                                     gene);

        if (_nb_gene == 1)
        {
          _initial_genome->insert (gene,
                                   GAListBASE::HEAD);
        }
        else
        {
          _initial_genome->insert (gene);
        }
      }

      current = g_slist_next (current);
    }
  }

  g_hash_table_foreach (_criteria_distribution,
                        (GHFunc) SetIdealData,
                        this);

  {
    genome.initializer (Initializer);

    GASteadyStateGA algo (genome);

    algo.initialize (SEED);

    algo.pReplacement   (0.5);
    algo.populationSize (100);
    algo.pMutation      (1.0);
    algo.pCrossover     (0.0);
    algo.nGenerations   (1000);
    algo.scaling        (GANoScaling ());
    //algo.terminator     (Terminator);
    algo.selectScores   (GAStatistics::AllScores);

    algo.evolve ();
    genome = algo.statistics().bestIndividual ();

    cout << "\n" << algo.statistics () << "\n";
    cout << "\nthe best score is ************************" << genome.score () << "\n";
  }

  // Pool leaders
  {
    GSList *current = fencer_list;

    for (guint i = 0; i < _nb_pools; i++)
    {
      _pool_table[i]._pool->AddFencer ((Player *) current->data,
                                       _owner);

      current = g_slist_next (current);
    }
  }

  // Reallocated fencers
  {
    Gene **gene = genome.head ();

    for (guint i = 0; i < _nb_gene; i++)
    {
      if (gene && *gene)
      {
        PoolData *pool_data = &_pool_table[GetPoolIndex (i)];

        pool_data->_pool->AddFencer ((*gene)->_fencer,
                                     _owner);
      }

      gene = genome.next ();
    }
  }
}

// --------------------------------------------------------------------------------
GeneticSwapper::~GeneticSwapper ()
{
  _criteria_id->Release ();
  g_rand_free (_rand);
  g_free (_pool_table);
  g_hash_table_destroy (_criteria_distribution );
  g_slist_free (_top_fencer_quark);
}

// --------------------------------------------------------------------------------
Swapper *GeneticSwapper::Create (GSList *pools,
                                 gchar  *criteria,
                                 GSList *fencer_list,
                                 Object *owner)
{
  return new GeneticSwapper (pools,
                             criteria,
                             fencer_list,
                             owner);
}

// --------------------------------------------------------------------------------
void GeneticSwapper::Delete ()
{
  Release ();
}

// --------------------------------------------------------------------------------
float GeneticSwapper::Objective (GAGenome &g)
{
  Genome         &genome = dynamic_cast <Genome &> (g);
  Gene           **gene  = genome.head ();
  float           score  = 0.0;
  GeneticSwapper *_this  = (GeneticSwapper *) g.userData ();

  if (gene)
  {
    score = _this->_nb_gene * (1 + _this->_nb_gene);

    {
      float swap_count = 0.0;

      for (guint i = 0; i < _this->_nb_gene; i++)
      {
        if (*gene)
        {
          guint     pool_index = _this->GetPoolIndex (i);
          PoolData *pool_data  = &_this->_pool_table[pool_index];

          //(*gene)->Dump (_this->_owner);
          {
            Player::AttributeId  previous_rank_attr_id ("previous_stage_rank", _this->_owner);
            Attribute *previous_stage_rank = (*gene)->_fencer->GetAttribute (&previous_rank_attr_id);

            //g_print ("%d %20ss >> %s (%d)\n", previous_stage_rank->GetUIntValue (), (*gene)->_fencer->GetName (), g_quark_to_string ((*gene)->_criteria_quark), pool_index);
          }

          // Init
          if (i <= _this->_nb_pools)
          {
            if (pool_data->_criteria_count)
            {
              g_hash_table_destroy (pool_data->_criteria_count);
            }
            pool_data->_criteria_count = g_hash_table_new (NULL,
                                                           NULL);
          }

          // swap count
          if ((*gene)->_original_pool != pool_index)
          {
            swap_count++;
          }

          // criteria
          {
            guint criteria_count = (guint) g_hash_table_lookup (pool_data->_criteria_count,
                                                                (const void *) (*gene)->_criteria_quark);
            criteria_count++;
            g_hash_table_insert (pool_data->_criteria_count,
                                 (void *) (*gene)->_criteria_quark,
                                 (void *) criteria_count);
          }
        }

        gene = genome.next ();
      }
      score -= swap_count;
    }

    {
      float error = 0.0;

      {
        GSList *current = _this->_top_fencer_quark;

        g_hash_table_foreach (_this->_criteria_distribution,
                              (GHFunc) ResetRealData,
                              NULL);

        while (current)
        {
          CriteriaData *criteria_data = (CriteriaData *) g_hash_table_lookup (_this->_criteria_distribution,
                                                                              current->data);
          criteria_data->_real_max_pool1++;

          current = g_slist_next (current);
        }
      }

      for (guint i = 0; i < _this->_nb_pools; i++)
      {
        PoolData       *pool_data = &_this->_pool_table[i];
        GHashTableIter  iter;
        GQuark          quark;
        guint           count;

        g_hash_table_iter_init (&iter,
                                pool_data->_criteria_count);

        while (g_hash_table_iter_next (&iter,
                                       (gpointer *) &quark,
                                       (gpointer *) &count))
        {
          //g_print ("%d >> %s - %d\n", i, g_quark_to_string (quark), count);
          CriteriaData *criteria_data = (CriteriaData *) g_hash_table_lookup (_this->_criteria_distribution,
                                                                              (const void *) quark);
          if (count == criteria_data->_ideal_max_fencer1)
          {
            criteria_data->_real_max_pool1++;

            if (criteria_data->_real_max_pool1 > criteria_data->_ideal_max_pool1)
            {
              error++;
              //g_print ("     %d >1> %s - %d\n", i, g_quark_to_string (quark), count);
            }
          }
          if (count == criteria_data->_ideal_max_fencer2)
          {
            criteria_data->_real_max_pool2++;

            if (criteria_data->_real_max_pool2 > criteria_data->_ideal_max_pool2)
            {
              error++;
              //g_print ("     %d >2> %s - %d\n", i, g_quark_to_string (quark), count);
            }
          }
        }
      }
      //g_print (">>>>>>> %f\n", error);
      score -= (error * _this->_nb_gene);
    }
  }

  return score;
}

// --------------------------------------------------------------------------------
void GeneticSwapper::Initializer (GAGenome &g)
{
  Genome         &genome = (Genome &) g;
  GeneticSwapper *_this  = (GeneticSwapper *) g.userData ();

  while (genome.head ())
  {
    genome.destroy ();
  }

  if (_this->_initial_genome)
  {
    g = *_this->_initial_genome;
    free (_this->_initial_genome);
    _this->_initial_genome = NULL;
  }
  else
  {
    GSList *list   = g_slist_copy (_this->_gene_list);
    guint   length = _this->_nb_gene;

    while (list)
    {
      GSList *choice;
      Gene   *gene;
      guint   rand = g_rand_int_range (_this->_rand,
                                       0, length);

      choice = g_slist_nth (list,
                            rand);

      if (length == _this->_nb_gene)
      {
        genome.insert ((Gene *) choice->data,
                       GAListBASE::HEAD);
      }
      else
      {
        genome.insert ((Gene *) choice->data);
      }

      gene = (Gene *) choice->data;

      list = g_slist_remove_link (list,
                                  choice);
      length--;
    }
  }
}

// --------------------------------------------------------------------------------
GABoolean GeneticSwapper::Terminator (GAGeneticAlgorithm &ga)
{
  if (   (ga.statistics().bestIndividual().score () == 0.0)
      || ga.generation() == 1000)
  {
    return gaTrue;
  }
  else
  {
    return gaFalse;
  }
}

// --------------------------------------------------------------------------------
void GeneticSwapper::InsertCriteria (GHashTable   *table,
                                     const GQuark  criteria)
{
  CriteriaData *criteria_data;

  criteria_data = (CriteriaData *) g_hash_table_lookup (table,
                                                        (const void *) criteria);
  if (criteria_data == NULL)
  {
    criteria_data = g_new (CriteriaData, 1);
    criteria_data->_count = 0;
    g_hash_table_insert (table,
                         (void *) criteria,
                         criteria_data);
  }
  criteria_data->_count++;
}

// --------------------------------------------------------------------------------
void GeneticSwapper::SetIdealData (GQuark          key,
                                   CriteriaData   *data,
                                   GeneticSwapper *swapper)
{
  data->_ideal_max_pool1 = data->_count / swapper->_nb_pools;
  data->_ideal_max_fencer1 = data->_count / swapper->_nb_pools;

  data->_ideal_max_pool2 = data->_count % swapper->_nb_pools;
  if (data->_ideal_max_pool2)
  {
    data->_ideal_max_fencer2 = data->_ideal_max_fencer1 + 1;
  }
  else
  {
    data->_ideal_max_fencer2 = 0;
  }
}

// --------------------------------------------------------------------------------
void GeneticSwapper::ResetRealData (GQuark          key,
                                    CriteriaData   *data,
                                    GeneticSwapper *swapper)
{
  data->_real_max_pool1 = 0;
  data->_real_max_pool2 = 0;
}

// --------------------------------------------------------------------------------
guint GeneticSwapper::GetPoolIndex (guint fencer_index)
{
  if (((fencer_index / _nb_pools) % 2) == 0)
  {
    return (_nb_pools-1 - fencer_index%_nb_pools);
  }
  else
  {
    return (fencer_index % _nb_pools);
  }
}

// --------------------------------------------------------------------------------
Player *GeneticSwapper::GetNextPlayer (Pool *for_pool)
{
  return NULL;
}
