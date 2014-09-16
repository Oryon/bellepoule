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

#include "dispatcher.hpp"
#include "default_order.data"

namespace Pool
{
  // --------------------------------------------------------------------------------
  Dispatcher::Dispatcher (const gchar *name)
    : Object ("Dispatcher")
  {
    _pool_size     = 0;
    _name          = g_strdup (name);
    _opponent_list = NULL;
    _pair_list     = NULL;
  }

  // --------------------------------------------------------------------------------
  Dispatcher::Dispatcher (guint        pool_size,
                          const gchar *name,
                          ...)
  {
    _pool_size     = pool_size;
    _name          = g_strdup (name);
    _opponent_list = NULL;
    _pair_list     = NULL;

    for (guint i = 0; i < pool_size; i++)
    {
      _opponent_list = g_list_append (_opponent_list,
                                      new Opponent (i+1, NULL));
    }

    {
      guint   pair_count = GetPairCount ();
      va_list ap;

      va_start (ap, name);
      for (guint i = 0; i < pair_count; i++)
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
  Dispatcher::~Dispatcher ()
  {
    Reset ();

    g_free (_name);
  }

  // --------------------------------------------------------------------------------
  void Dispatcher::Reset ()
  {
    g_list_free_full (_pair_list,
                      (GDestroyNotify) Object::TryToRelease);
    _pair_list = NULL;

    g_list_free_full (_opponent_list,
                      (GDestroyNotify) Object::TryToRelease);
    _opponent_list = NULL;;
  }

  // --------------------------------------------------------------------------------
  guint Dispatcher::GetPairCount ()
  {
    if (_pool_size > 0)
    {
      return (_pool_size * (_pool_size - 1)) / 2;
    }

    return 0;
  }

  // --------------------------------------------------------------------------------
  void Dispatcher::SetAffinityCriteria (AttributeDesc *affinity_criteria,
                                        GSList        *fencer_list)
  {
    guint       team_count;
    GHashTable *affinity_distribution = g_hash_table_new (NULL,
                                                          NULL);

    Reset ();

    _pool_size = g_slist_length (fencer_list);

    {
      Player::AttributeId *affinity = Player::AttributeId::Create (affinity_criteria, NULL);
      GSList              *current  = fencer_list;

      for (guint i = 0; current != NULL; i++)
      {
        Fencer   *fencer   = (Fencer *) current->data;
        Opponent *opponent = new Opponent (i+1, fencer);

        _opponent_list = g_list_append (_opponent_list,
                                        opponent);

        if (affinity_criteria)
        {
          Attribute *criteria_attr = fencer->GetAttribute (affinity);

          if (criteria_attr)
          {
            gchar  *user_image    = criteria_attr->GetUserImage (AttributeDesc::LONG_TEXT);
            GQuark  quark         = g_quark_from_string (user_image);
            GSList *affinity_list;

            opponent->SetQuark (quark);

            affinity_list = (GSList *) g_hash_table_lookup (affinity_distribution,
                                                            (const void *) quark);
            affinity_list = g_slist_append (affinity_list,
                                            (void *) i);
            g_hash_table_insert (affinity_distribution,
                                 (void *) quark,
                                 affinity_list);

            g_free (user_image);
          }
        }

        current = g_slist_next (current);
      }
      Object::TryToRelease (affinity);
    }

    // Lock by affinity
    {
      GHashTableIter  iter;
      GSList         *affinity_list;
      GQuark          quark;

      g_hash_table_iter_init (&iter,
                              affinity_distribution);
      while (g_hash_table_iter_next (&iter,
                                     (gpointer *) &quark,
                                     (gpointer *) &affinity_list))
      {
        LockOpponents (quark,
                       g_slist_length (affinity_list));
      }
    }

    team_count = g_hash_table_size (affinity_distribution);
    if (   (team_count == 0)
        || (team_count == 1)
        || (team_count == _pool_size))
    {
      if ((_pool_size >= 2) &&  (_pool_size <= _MAX_POOL_SIZE))
      {
        guint       pair_count = GetPairCount ();
        PlayerPair *pair_table = fencing_pairs[_pool_size];

        for (guint i = pair_count; i > 0; i--)
        {
          Pair *pair = new Pair (i,
                                 (Opponent *) g_list_nth_data (_opponent_list, pair_table[i-1]._a-1),
                                 (Opponent *) g_list_nth_data (_opponent_list, pair_table[i-1]._b-1));

          _pair_list = g_list_prepend (_pair_list,
                                       pair);
        }
        RefreshFitness ();
      }
    }
    else
    {
      SpreadOpponents (_opponent_list);
      CreatePairs     (_opponent_list);
      RefreshFitness ();

#ifdef DEBUG_DISPATCHER
      if (_pool_size > 4)
      {
        FixErrors ();
        RefreshFitness ();
      }
#endif
    }

    g_hash_table_destroy (affinity_distribution);
  }

  // --------------------------------------------------------------------------------
  void Dispatcher::LockOpponents (GQuark teammate_quark,
                                  guint  teammate_count)
  {
    if (teammate_quark && (teammate_count > 1))
    {
      GList *current = _opponent_list;

      while (current)
      {
        Opponent *o = (Opponent *) current->data;

        o->Lock (teammate_quark,
                 teammate_count);
        current = g_list_next (current);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Dispatcher::RefreshFitness ()
  {
    GList *current = g_list_last (_pair_list);

    g_list_foreach (_opponent_list,
                    (GFunc) Opponent::ResetFitnessProfile,
                    GUINT_TO_POINTER (GetPairCount ()));

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
  void Dispatcher::SpreadOpponents (GList *opponent_list)
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
  void Dispatcher::CreatePairs (GList *opponent_list)
  {
    GList *working_list = g_list_copy (opponent_list);
    GList *current      = working_list;
    gint   iteration    = 1;

    while (current)
    {
      Opponent *a = (Opponent *) current->data;
      Opponent *b = a->GetBestOpponent ();

      if (b && ((b->GetFitness () == -1) || (b->GetFitness () < iteration-1)))
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
    g_list_free (working_list);
  }

  // --------------------------------------------------------------------------------
  void Dispatcher::FixErrors ()
  {
#if 0
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
#endif
  }

  // --------------------------------------------------------------------------------
  gboolean Dispatcher::GetPlayerPair (guint     match_index,
                                      guint    *a_id,
                                      guint    *b_id,
                                      gboolean *rest_error)
  {
    Pair *pair = (Pair *) g_list_nth_data (_pair_list,
                                           match_index);

    if (pair)
    {
      *a_id = pair->GetA ();
      *b_id = pair->GetB ();

      *rest_error = pair->HasFitnessError ();

      return TRUE;
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void Dispatcher::Dump ()
  {
#ifdef DEBUG_DISPATCHER
    guint  pair_count    = GetPairCount ();
    guint  max_fitness   = 0;
    guint *fitness_table = g_new0 (guint, pair_count);

    printf (GREEN "**** %s\n" ESC, _name);

    // Flat list
    {
      GList *current = _pair_list;

      for (guint i = 1; current != NULL; i++)
      {
        Pair  *pair      = (Pair *) current->data;
        guint  a_fitness = pair->GetAFitness ();
        guint  b_fitness = pair->GetBFitness ();

        pair->Dump ();

        if (a_fitness < pair_count) fitness_table[a_fitness]++;
        if (b_fitness < pair_count) fitness_table[b_fitness]++;

        if (a_fitness > max_fitness) max_fitness = a_fitness;
        if (b_fitness > max_fitness) max_fitness = b_fitness;

        current = g_list_next (current);
      }
    }

    // Fitness distribution
    {
      GList *current = _opponent_list;

      printf ("   ");
      for (guint i = 0; i < max_fitness+1; i++)
      {
        printf ("; Rest %2d", i);
      }
      printf ("\n");

      while (current)
      {
        Opponent *opponent = (Opponent *) current->data;

        opponent->Dump (max_fitness+1);

        current = g_list_next (current);
      }
    }

    // Global digest
    for (guint i = 0; i < max_fitness+1; i++)
    {
      if (fitness_table[i])
      {
        printf (BLUE);
      }
      printf ("%d:%2d   " ESC, i, fitness_table[i]);
    }

    printf ("\n\n");

    g_free (fitness_table);
#endif
  }
}
