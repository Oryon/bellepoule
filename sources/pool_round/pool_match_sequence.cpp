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

#include "pool_match_sequence.hpp"

namespace Pool
{
  // --------------------------------------------------------------------------------
  Opponent::Opponent (guint id)
    : Object ("Opponent")
  {
    _id            = id;
    _fitness       = 0;
    _opponent_list = NULL;
    _pair_list     = NULL;
  }

  // --------------------------------------------------------------------------------
  Opponent::~Opponent ()
  {
    g_list_free (_opponent_list);
    g_list_free (_pair_list);
  }

  // --------------------------------------------------------------------------------
  void Opponent::Feed (GList *opponent_list)
  {
    _opponent_list = g_list_copy (opponent_list);

    _opponent_list = g_list_delete_link (_opponent_list,
                                         g_list_nth (_opponent_list, _id-1));
  }

  // --------------------------------------------------------------------------------
  Opponent *Opponent::GetBestOpponent ()
  {
    _opponent_list = g_list_sort (_opponent_list,
                                  (GCompareFunc) CompareFitness);

    if (_opponent_list)
    {
      Opponent *target = (Opponent *) _opponent_list->data;

      _opponent_list = g_list_delete_link (_opponent_list,
                                           _opponent_list);
      target->_opponent_list = g_list_delete_link (target->_opponent_list,
                                                   g_list_find (target->_opponent_list, this));

      return target;
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  void Opponent::AddPair (Pair  *pair,
                          guint  fitness)
  {
    _fitness = fitness;
    _pair_list = g_list_prepend (_pair_list,
                                 pair);
  }

  // --------------------------------------------------------------------------------
  gint Opponent::CompareFitness (Opponent *a,
                                 Opponent *b)
  {
    return a->_fitness - b->_fitness;
  }

  // --------------------------------------------------------------------------------
  void Opponent::Dump ()
  {
  }
}

namespace Pool
{
  // --------------------------------------------------------------------------------
  Pair::Pair (guint     iteration,
              Opponent *a,
              Opponent *b)
  {
    _a = a;
    _b = b;

    _a->AddPair (this, iteration);
    _b->AddPair (this, iteration);
  }

  // --------------------------------------------------------------------------------
  Pair::~Pair ()
  {
  }

  // --------------------------------------------------------------------------------
  void Pair::Dump ()
  {
    if (_fitness == 0)
    {
      printf (RED);
      printf ("%2d - %2d  : %2d\n" ESC, _a->_id, _b->_id, _fitness);
    }
  }

  // --------------------------------------------------------------------------------
  void Pair::SetFitness (guint fitness)
  {
    _fitness = fitness;
  }

  // --------------------------------------------------------------------------------
  guint Pair::GetFitness ()
  {
    return _fitness;
  }

  // --------------------------------------------------------------------------------
  gboolean Pair::HasSameOpponent (Pair *than)
  {
    if (than)
    {
      return (   (_a == than->_a)
              || (_a == than->_b)
              || (_b == than->_a)
              || (_b == than->_b));
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  gint Pair::CompareFitness (GList *a,
                             GList *b)
  {
    return ((Pair *) b->data)->_fitness - ((Pair *) a->data)->_fitness;
  }
}

namespace Pool
{
  // --------------------------------------------------------------------------------
  MatchSequence::MatchSequence (guint pool_size)
    : Object ("PoolMatchSequence")
  {
    GList *opponent_list = GetOpponentList (pool_size);

    _pair_list = NULL;

    SpreadOpponents (opponent_list);
    CreatePairs (opponent_list);

    RefreshFitness ();
    //Dump ();

    if (pool_size > 4)
    {
      FixErrors ();

      RefreshFitness ();
      Dump ();
    }

    g_list_free_full (opponent_list,
                      (GDestroyNotify) Object::TryToRelease);
  }

  // --------------------------------------------------------------------------------
  MatchSequence::~MatchSequence ()
  {
    g_list_free_full (_pair_list,
                      (GDestroyNotify) Object::TryToRelease);
  }

  // --------------------------------------------------------------------------------
  guint MatchSequence::GetFitness (Pair *pair)
  {
    guint  fitness   = 0;
    GList *pair_node = g_list_find (_pair_list, pair);
    GList *current;

    current = g_list_next (pair_node);
    for (guint i = 0; current != NULL; i++)
    {
      Pair *current_pair = (Pair *) current->data;

      if (current_pair->HasSameOpponent (pair))
      {
        fitness = i;
        break;
      }

      current = g_list_next (current);
    }

    current = g_list_previous (pair_node);
    for (guint i = 0; current != NULL; i++)
    {
      Pair *current_pair = (Pair *) current->data;

      if (current_pair->HasSameOpponent (pair))
      {
        if (fitness)
        {
          fitness = MIN (fitness, i);
        }
        else
        {
          fitness = i;
        }
        break;
      }

      current = g_list_previous (current);
    }

    return fitness;
  }

  // --------------------------------------------------------------------------------
  void MatchSequence::RefreshFitness ()
  {
    GList *current = _pair_list;

    for (guint i = 1; current != NULL; i++)
    {
      Pair  *pair    = (Pair *) current->data;
      guint  fitness = GetFitness (pair);

      pair->SetFitness (fitness);

      current = g_list_next (current);
    }
  }

  // --------------------------------------------------------------------------------
  void MatchSequence::Dump ()
  {
    GList *current = _pair_list;

    for (guint i = 1; current != NULL; i++)
    {
      Pair *pair = (Pair *) current->data;

      pair->Dump ();

      current = g_list_next (current);
    }
    printf ("\n");
  }

  // --------------------------------------------------------------------------------
  GList *MatchSequence::GetOpponentList (guint pool_size)
  {
    GList *opponent_list = NULL;

    for (guint i = 0; i < pool_size; i++)
    {
      Opponent *opponent = new Opponent (i+1);

      opponent_list = g_list_append (opponent_list,
                                     opponent);
    }

    return opponent_list;
  }

  // --------------------------------------------------------------------------------
  void MatchSequence::SpreadOpponents (GList *opponent_list)
  {
    GList *current = opponent_list;

    while (current)
    {
      Opponent *opponent = (Opponent *) current->data;

      opponent->Feed (opponent_list);
      current = g_list_next (current);
    }
  }

  // --------------------------------------------------------------------------------
  void MatchSequence::CreatePairs (GList *opponent_list)
  {
    GList *current   = opponent_list;
    guint  iteration = 1;

    while (current)
    {
      Opponent *a = (Opponent *) current->data;
      Opponent *b = a->GetBestOpponent ();

      if (b)
      {
        _pair_list = g_list_append (_pair_list,
                                    new Pair (iteration, a, b));

        opponent_list = g_list_sort (opponent_list,
                                     (GCompareFunc) Opponent::CompareFitness);
        current = opponent_list;
        iteration++;
      }
      else
      {
        current = g_list_next (current);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void MatchSequence::FixErrors ()
  {
    GList *current = _pair_list;

    while (current)
    {
      Pair *current_pair = (Pair *) current->data;

      if (current_pair->GetFitness () == 0)
      {
        GList *tail = g_list_copy (current);

        while (current)
        {
          GList *next = g_list_next (current);

          _pair_list = g_list_remove_link (_pair_list,
                                           current);
          current = next;
        }
        _pair_list = g_list_concat (tail,
                                    _pair_list);
        break;
      }

      current = g_list_next (current);
    }
  }
}
