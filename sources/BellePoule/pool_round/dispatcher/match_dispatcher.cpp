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

#include "match_dispatcher.hpp"

namespace Pool
{
  // --------------------------------------------------------------------------------
  MatchDispatcher::MatchDispatcher (guint pool_size)
    : Object ("PoolMatchSequence")
  {
    Init ("Generated",
          pool_size);

    {
      SpreadOpponents (_opponent_list);
      CreatePairs (_opponent_list);
    }

    //Dump ();

    if (pool_size > 4)
    {
      RefreshFitness ();
      FixErrors ();
    }
  }

  // --------------------------------------------------------------------------------
  MatchDispatcher::MatchDispatcher (guint        pool_size,
                                    const gchar *name,
                                    ...)
  {
    Init (name,
          pool_size);

    {
      va_list ap;

      va_start (ap, name);
      for (guint i = 0; i < ((pool_size * (pool_size-1)) / 2); i++)
      {
        guint a = va_arg (ap, guint);
        guint b = va_arg (ap, guint);

        _pair_list = g_list_append (_pair_list,
                                    new Pair (i,
                                              (Opponent *) g_list_nth_data (_opponent_list, a-1),
                                              (Opponent *) g_list_nth_data (_opponent_list, b-1)));
      }
      va_end (ap);
    }
  }

  // --------------------------------------------------------------------------------
  MatchDispatcher::~MatchDispatcher ()
  {
    g_list_free_full (_pair_list,
                      (GDestroyNotify) Object::TryToRelease);

    g_list_free_full (_opponent_list,
                      (GDestroyNotify) Object::TryToRelease);

    g_free (_name);
  }

  // --------------------------------------------------------------------------------
  void MatchDispatcher::Init (const gchar *name,
                              guint        pool_size)
  {
    _pair_list = NULL;
    _pool_size = pool_size;
    _name      = g_strdup (name);

    _opponent_list = NULL;
    for (guint i = 0; i < pool_size; i++)
    {
      _opponent_list = g_list_append (_opponent_list,
                                      new Opponent (i+1));
    }
  }

  // --------------------------------------------------------------------------------
  void MatchDispatcher::RefreshFitness ()
  {
    GList *current = g_list_last (_pair_list);

    g_list_foreach (_opponent_list,
                    (GFunc) Opponent::ResetFitnessProfile,
                    GUINT_TO_POINTER (_pool_size));

    while (current)
    {
      Pair  *pair     = (Pair *) current->data;
      GList *previous = g_list_previous (current);

      pair->ResetFitness ();

      for (guint i = 0; previous != NULL; i++)
      {
        Pair *previous_pair = (Pair *) previous->data;

        pair->SetFitness (previous_pair,
                          i);

        previous = g_list_previous (previous);
      }

      current = g_list_previous (current);
    }
  }

  // --------------------------------------------------------------------------------
  void MatchDispatcher::Dump ()
  {
    guint *fitness_table = g_new0 (guint, _pool_size);

    printf (GREEN "**** %s\n" ESC, _name);

    RefreshFitness ();

    // Flat list
    {
      GList *current = _pair_list;

      for (guint i = 1; current != NULL; i++)
      {
        Pair  *pair      = (Pair *) current->data;
        guint  a_fitness = pair->GetAFitness ();
        guint  b_fitness = pair->GetBFitness ();

        pair->Dump ();

        if (a_fitness < _pool_size) fitness_table[a_fitness]++;
        if (b_fitness < _pool_size) fitness_table[b_fitness]++;

        current = g_list_next (current);
      }
    }

    // Fitness distribution
    {
      GList *current = _opponent_list;

      printf ("   ");
      for (guint i = 0; i < _pool_size; i++)
      {
        printf ("; Repos %2d", i);
      }
      printf ("\n");

      while (current)
      {
        Opponent *opponent = (Opponent *) current->data;

        opponent->Dump ();

        current = g_list_next (current);
      }
    }

    // Global digest
    for (guint i = 0; i < _pool_size; i++)
    {
      if (fitness_table[i])
      {
        printf (BLUE);
      }
      printf ("%d:%2d   " ESC, i, fitness_table[i]);
    }

    printf ("\n\n");

    g_free (fitness_table);
  }

  // --------------------------------------------------------------------------------
  void MatchDispatcher::SpreadOpponents (GList *opponent_list)
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
  void MatchDispatcher::CreatePairs (GList *opponent_list)
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
  void MatchDispatcher::FixErrors ()
  {
    GList *current = _pair_list;

    while (current)
    {
      Pair *current_pair = (Pair *) current->data;

      if (current_pair->HasFitnessError ())
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
