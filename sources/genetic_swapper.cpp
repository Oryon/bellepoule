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
#include <ga/GAListGenome.h>
#include <ga/garandom.h>
#include <ga/std_stream.h>

#include "object.hpp"
#include "pool.hpp"

#include "genetic_swapper.hpp"

#define cout STD_COUT

GRand                    *GeneticSwapper::_rand           = NULL;
guint                     GeneticSwapper::_nb_pools       = 0;
GeneticSwapper::PoolData *GeneticSwapper::_pool_table     = NULL;
guint                     GeneticSwapper::_nb_fencers     = 0;
GSList                   *GeneticSwapper::_fencer_list    = NULL;
GeneticSwapper::Genome   *GeneticSwapper::_initial_genome = NULL;

// --------------------------------------------------------------------------------
void GeneticSwapper::Fencer::Dump ()
{
  g_print ("%s >> %d\n", _fencer->GetName (), _criteria_value);
}

// --------------------------------------------------------------------------------
bool GeneticSwapper::Fencer::operator == (Fencer const &f)
{
  return (_fencer == f._fencer);
}

// --------------------------------------------------------------------------------
bool GeneticSwapper::Fencer::operator != (Fencer const &f)
{
  return (_fencer != f._fencer);
}

// --------------------------------------------------------------------------------
float GeneticSwapper::Objective (GAGenome &g)
{
  Genome &genome  = (Genome &) g;
  Fencer **fencer = genome.head ();
  float   score   = 0.0;

  if (fencer)
  {
    for (guint i = 0; i < _nb_fencers; i++)
    {
      if (*fencer)
      {
        guint     pool_index = GetPoolIndex (i);
        PoolData *pool_data  = &_pool_table[pool_index];

        // (*fencer)->Dump ();

        if (i <= _nb_pools)
        {
          pool_data->_error = 0;
        }

        if ((*fencer)->_original_pool != pool_index)
        {
          score++;
        }
      }

      fencer = genome.next ();
    }

    for (guint i = 0; i < _nb_pools; i++)
    {
      PoolData *pool = &_pool_table[i];

      score += pool->_error;
    }
  }

  return score;
}

// --------------------------------------------------------------------------------
void GeneticSwapper::Initializer (GAGenome &g)
{
  Genome &genome = (Genome &) g;

  while (genome.head ())
  {
    genome.destroy ();
  }

  if (_initial_genome)
  {
    g = *_initial_genome;
    free (_initial_genome);
    _initial_genome = NULL;
  }
  else
  {
    GSList *list   = g_slist_copy (_fencer_list);
    guint   length = _nb_fencers;

    while (list)
    {
      GSList *choice;
      Fencer *fencer;
      guint   rand = g_rand_int_range (_rand,
                                       0, length);

      choice = g_slist_nth (list,
                            rand);

      if (length == _nb_fencers)
      {
        genome.insert ((Fencer *) choice->data,
                       GAListBASE::HEAD);
      }
      else
      {
        genome.insert ((Fencer *) choice->data);
      }

      fencer = (Fencer *) choice->data;

      list = g_slist_remove_link (list,
                                  choice);
      length--;
    }
  }
}

// --------------------------------------------------------------------------------
GeneticSwapper::GeneticSwapper (GSList *pools,
                                gchar  *criteria,
                                GSList *fencer_list,
                                Object *owner)
: Object ("GeneticSwapper")
{
  Genome best_genome;

  _rand = g_rand_new_with_seed (_seed);

  {
    _nb_pools   = g_slist_length (pools);
    _pool_table = g_new (PoolData,
                         _nb_pools);

    {
      GSList *current = pools;

      for (guint i = 0; i < _nb_pools; i++)
      {
        _pool_table[i]._pool = (Pool *) current->data;

        current = g_slist_next (current);
      }
    }
  }

  _initial_genome = new Genome ();

  _fencer_list = NULL;
  _nb_fencers  = 0;

  {
    // Prevent the pool leader fencers from being moved
    GSList *current = g_slist_nth (fencer_list,
                                   _nb_pools);

    _criteria_id = new Player::AttributeId (criteria);
    while (current)
    {
      Fencer *fencer = new Fencer;

      fencer->_fencer = (Player *) current->data;

      // criteria
      {
        Attribute *criteria_attr = fencer->_fencer->GetAttribute (_criteria_id);

        if (criteria_attr)
        {
          fencer->_criteria_value = g_quark_from_string (criteria_attr->GetUserImage ());
        }
        else
        {
          fencer->_criteria_value = 0;
        }
      }

      _nb_fencers++;
      _fencer_list = g_slist_append (_fencer_list,
                                     fencer);

      if (_nb_fencers == 1)
      {
        _initial_genome->insert (fencer,
                                 GAListBASE::HEAD);
      }
      else
      {
        _initial_genome->insert (fencer);
      }

      current = g_slist_next (current);
    }
  }

  {
    Genome genome (Objective);
    genome.initializer (Initializer);

    GASteadyStateGA algo (genome);

    algo.minimize ();
#if 1
    algo.pReplacement   (1.0);
    algo.populationSize (100);
    algo.nGenerations   (500);
    algo.pMutation      (0.1);
    algo.pCrossover     (1.0);
#else
    algo.pReplacement   (1.0);
    algo.populationSize (3);
    algo.nGenerations   (5);
    algo.pMutation      (1.0);
    //algo.pCrossover     (1.0);
#endif
    algo.selectScores (GAStatistics::AllScores);

    algo.initialize (_seed);

    g_print ("evolving...\n");
    while (!algo.done ())
    {
      algo.step ();

      if (algo.generation () % 10 == 0)
      {
        cout << algo.generation () << " "; cout.flush();
      }
    }

    best_genome = algo.statistics().bestIndividual ();

    cout << "the best score is " << best_genome.score () << "\n";
    cout << algo.statistics() << "\n";
  }

  // Pool leaders
  {
    GSList *current = fencer_list;

    for (guint i = 0; i < _nb_pools; i++)
    {
      _pool_table[i]._pool->AddFencer ((Player *) current->data,
                                       owner);

      current = g_slist_next (current);
    }
  }

  // Reallocated fencers
  {
    Fencer **fencer = best_genome.head ();

    for (guint i = 0; i < _nb_fencers; i++)
    {
      if (fencer && *fencer)
      {
        PoolData *pool_data = &_pool_table[GetPoolIndex (i)];

        pool_data->_pool->AddFencer ((*fencer)->_fencer,
                                     owner);
      }

      fencer = best_genome.next ();
    }
  }
}

// --------------------------------------------------------------------------------
GeneticSwapper::~GeneticSwapper ()
{
  _criteria_id->Release ();
  g_rand_free (_rand);
  g_free (_pool_table);
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
