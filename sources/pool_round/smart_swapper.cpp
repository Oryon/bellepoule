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

#include "object.hpp"
#include "pool.hpp"
#include "pool_zone.hpp"

#include "smart_swapper.hpp"

// --------------------------------------------------------------------------------
void SmartSwapper::Fencer::Dump (Object *owner)
{
  Player::AttributeId  previous_rank_attr_id ("previous_stage_rank", owner);
  Attribute *previous_stage_rank = _player->GetAttribute (&previous_rank_attr_id);

  g_print ("%d %20ss >> %s (pool #%d)\n", previous_stage_rank->GetUIntValue (), _player->GetName (), g_quark_to_string (_criteria_quark), _original_pool);
}

// --------------------------------------------------------------------------------
SmartSwapper::SmartSwapper (Object *owner)
: Object ("SmartSwapper")
{
  _owner = owner;
}

// --------------------------------------------------------------------------------
SmartSwapper::~SmartSwapper ()
{
}

// --------------------------------------------------------------------------------
Swapper *SmartSwapper::Create (Object *owner)
{
  return new SmartSwapper (owner);
}

// --------------------------------------------------------------------------------
void SmartSwapper::Delete ()
{
  Release ();
}

// --------------------------------------------------------------------------------
void SmartSwapper::Swap (GSList *zones,
                         gchar  *criteria,
                         GSList *fencer_list)
{
  _criteria_id = new Player::AttributeId (criteria);

  _nb_pools   = 0;
  _pool_table = NULL;

  _error_list    = NULL;
  _floating_list = NULL;

  CreatePoolTable (zones);

  {
    guint nb_fencer = g_slist_length (fencer_list);

    _min_pool_size = nb_fencer / _nb_pools;
    _max_pool_size = _min_pool_size;
    if (nb_fencer % _nb_pools)
    {
      _max_pool_size++;
    }
  }

  LookUpDistribution (fencer_list);

  ExtractErrors    ();
  ExtractFloatings ();

  DispatchErrors    ();
  DispatchFloatings ();

  StoreSwapping ();

  _criteria_id->Release ();
  DeletePoolTable ();
  g_hash_table_destroy (_criteria_distribution);
}

// --------------------------------------------------------------------------------
void SmartSwapper::StoreSwapping ()
{
  for (guint i = 0; i < _nb_pools; i++)
  {
    PoolData *pool_data = &_pool_table[i];
    GList    *current   = pool_data->_fencer_list;

    while (current)
    {
      Fencer *fencer = (Fencer *) current->data;

      //fencer->_player->SetData (_owner,
                               //"original_pool",
                               //(void *) i);
      g_print ("\n");
      fencer->Dump (_owner);
      pool_data->_pool->AddFencer (fencer->_player,
                                   _owner);

      current = g_list_next (current);
    }
  }
}

// --------------------------------------------------------------------------------
void SmartSwapper::CreatePoolTable (GSList *zones)
{
  DeletePoolTable ();

  _nb_pools   = g_slist_length (zones);
  _pool_table = g_new (PoolData,
                       _nb_pools);

  {
    GSList *current = zones;

    for (guint i = 0; i < _nb_pools; i++)
    {
      PoolZone *zone = (PoolZone *) current->data;

      _pool_table[i]._pool           = zone->GetPool ();
      _pool_table[i]._fencer_list    = NULL;
      _pool_table[i]._criteria_score = g_hash_table_new (NULL,
                                                         NULL);
      current = g_slist_next (current);
    }
  }
}

// --------------------------------------------------------------------------------
void SmartSwapper::DeletePoolTable ()
{
  for (guint i = 0; i < _nb_pools; i++)
  {
    g_list_free (_pool_table[i]._fencer_list);
  }
  _nb_pools = 0;

  g_free (_pool_table);
}

// --------------------------------------------------------------------------------
void SmartSwapper::LookUpDistribution (GSList *fencer_list)
{
  _criteria_distribution = g_hash_table_new_full (NULL,
                                                  NULL,
                                                  NULL,
                                                  g_free);

  {
    GSList *current = fencer_list;

    for (guint i = 0; current != NULL; i++)
    {
      Player *player = (Player *) current->data;
      Fencer *fencer = new Fencer;
      GQuark  quark  = 0;

      // criteria
      {
        Attribute *criteria_attr = player->GetAttribute (_criteria_id);

        if (criteria_attr)
        {
          quark = g_quark_from_string (criteria_attr->GetUserImage ());

          InsertCriteria (_criteria_distribution,
                          quark);
        }
      }

      fencer->_player         = player;
      fencer->_criteria_quark = quark;
      fencer->_original_pool  = GetPoolIndex (i);

      // criteria score for the current pool
      {
        PoolData *pool_data = &_pool_table[fencer->_original_pool];
        guint     score;

        fencer->Dump (_owner);
        pool_data->_fencer_list = g_list_append (pool_data->_fencer_list,
                                                 fencer);

        score = (guint) g_hash_table_lookup (pool_data->_criteria_score,
                                             (const void *) fencer->_criteria_quark);
        score++;

        g_hash_table_insert (pool_data->_criteria_score,
                             (void *) fencer->_criteria_quark,
                             (void *) score);
      }

      current = g_slist_next (current);
    }
  }

  g_hash_table_foreach (_criteria_distribution,
                        (GHFunc) SetExpectedData,
                        this);
}

// --------------------------------------------------------------------------------
void SmartSwapper::ExtractErrors ()
{
  GHashTableIter iter;
  guint          count;
  GQuark         quark;

  for (guint i = 0; i < _nb_pools; i++)
  {
    PoolData *pool_data = &_pool_table[i];

    {
      g_hash_table_iter_init (&iter,
                              pool_data->_criteria_score);

      g_print ("---%d----\n", i);
      while (g_hash_table_iter_next (&iter,
                                     (gpointer *) &quark,
                                     (gpointer *) &count))
      {
        CriteriaData *criteria_data = (CriteriaData *) g_hash_table_lookup (_criteria_distribution,
                                                                            (const void *) quark);

        if (count > criteria_data->_max_criteria_occurrence)
        {
          ExtractFencers (pool_data,
                          quark,
                          count - criteria_data->_max_criteria_occurrence,
                          &_error_list);
          criteria_data->_has_errors = TRUE;
          criteria_data->_floating_fencers++;
        }

        if (criteria_data->_max_floating_fencers && (count == criteria_data->_max_criteria_occurrence))
        {
          criteria_data->_floating_fencers++;
        }
      }
    }
  }

  for (guint i = 0; i < _nb_pools; i++)
  {
    PoolData *pool_data = &_pool_table[i];

    g_hash_table_iter_init (&iter,
                            pool_data->_criteria_score);

    while (g_hash_table_iter_next (&iter,
                                   (gpointer *) &quark,
                                   (gpointer *) &count))
    {
      CriteriaData *criteria_data = (CriteriaData *) g_hash_table_lookup (_criteria_distribution,
                                                                          (const void *) quark);

      if (criteria_data->_floating_fencers > criteria_data->_max_floating_fencers)
      {
        if (count == criteria_data->_max_criteria_occurrence)
        {
          ExtractFencers (pool_data,
                          quark,
                          1,
                          &_error_list);
          criteria_data->_floating_fencers--;
        }
      }
    }
  }
}

// --------------------------------------------------------------------------------
void SmartSwapper::ExtractFloatings ()
{
  for (guint i = 0; i < _nb_pools; i++)
  {
    PoolData       *pool_data = &_pool_table[i];
    GHashTableIter  iter;
    GQuark          quark;
    guint           count;

    g_hash_table_iter_init (&iter,
                            pool_data->_criteria_score);

    g_print ("\n===%d====\n", i);
    while (g_hash_table_iter_next (&iter,
                                   (gpointer *) &quark,
                                   (gpointer *) &count))
    {
      CriteriaData *criteria_data = (CriteriaData *) g_hash_table_lookup (_criteria_distribution,
                                                                          (const void *) quark);

      if ((criteria_data->_has_errors == FALSE) && criteria_data->_max_floating_fencers)
      {
        if (count == criteria_data->_max_criteria_occurrence)
        {
          ExtractFencers (pool_data,
                          quark,
                          1,
                          &_floating_list);
        }
      }
    }
  }
}

// --------------------------------------------------------------------------------
void SmartSwapper::ExtractFencers (PoolData  *from_pool,
                                   GQuark     with_criteria,
                                   guint      number,
                                   GSList   **to_list)
{
  GList *current = g_list_last (from_pool->_fencer_list);

  for (guint i = 0; i < number; i++)
  {
    Fencer *fencer = (Fencer *) current->data;

    while (fencer->_criteria_quark != with_criteria)
    {
      current = g_list_previous (current);
      if (current == NULL)
      {
        return;
      }
      fencer = (Fencer *) current->data;
    }

    from_pool->_fencer_list = g_list_remove_link (from_pool->_fencer_list,
                                                  current);
    *to_list = g_slist_append (*to_list,
                               current->data);

    {
      Fencer *fencer = (Fencer *) current->data;

      fencer->Dump (_owner);
    }

    current = g_list_previous (current);
  }

  {
    guint score = (guint) g_hash_table_lookup (from_pool->_criteria_score,
                                               (const void *) with_criteria);
    score -= number;

    g_hash_table_insert (from_pool->_criteria_score,
                         (void *) with_criteria,
                         (void *) score);
  }
}

// --------------------------------------------------------------------------------
void SmartSwapper::DispatchErrors ()
{
  g_print ("\n\nERRORS\n");
  DispatchFencers (_error_list,
                   FALSE);

  g_slist_free (_error_list);
  _error_list = NULL;
}

// --------------------------------------------------------------------------------
void SmartSwapper::DispatchFloatings ()
{
  g_print ("\n\nFLOATING\n");
  DispatchFencers (_floating_list,
                   TRUE);

  g_slist_free (_floating_list);
  _floating_list = NULL;
}

// --------------------------------------------------------------------------------
void SmartSwapper::DispatchFencers (GSList   *list,
                                    gboolean  try_original_pool_first)
{
  const guint score_goals[] = {_min_pool_size, _max_pool_size};

  while (list)
  {
    guint         pool_index;
    Fencer       *fencer        = (Fencer *) list->data;
    CriteriaData *criteria_data = (CriteriaData *) g_hash_table_lookup (_criteria_distribution,
                                                                        (const void *) fencer->_criteria_quark);

    if (try_original_pool_first)
    {
      pool_index = fencer->_original_pool;
      for (guint goal = 0; goal < sizeof (score_goals); goal++)
      {
        PoolData *pool_data = &_pool_table[pool_index];

        if (FencerCanGoTo (fencer,
                           pool_data,
                           criteria_data,
                           score_goals[goal]))
        {
          goto next_fencer;
        }
      }
    }

    for (guint goal = 0; goal < sizeof (score_goals); goal++)
    {
      for (pool_index = 0; pool_index < _nb_pools; pool_index++)
      {
        PoolData *pool_data = &_pool_table[pool_index];

        if (FencerCanGoTo (fencer,
                           pool_data,
                           criteria_data,
                           score_goals[goal]))
        {
          goto next_fencer;
        }
      }
    }

next_fencer:
    fencer->Dump (_owner);
    g_print ("          %d\n", pool_index);

    {
      PoolData *pool_data = &_pool_table[pool_index];

      guint score = (guint) g_hash_table_lookup (pool_data->_criteria_score,
                                                 (const void *) fencer->_criteria_quark);
      score++;

      g_hash_table_insert (pool_data->_criteria_score,
                           (void *) fencer->_criteria_quark,
                           (void *) score);

      pool_data->_fencer_list = g_list_prepend (pool_data->_fencer_list,
                                                fencer);
    }

    list = g_slist_next (list);
  }
}

// --------------------------------------------------------------------------------
gboolean SmartSwapper::FencerCanGoTo (Fencer       *fencer,
                                      PoolData     *pool_data,
                                      CriteriaData *criteria_data,
                                      guint         goal)
{
  if (g_list_length (pool_data->_fencer_list) < goal)
  {
    guint score;

    score = (guint) g_hash_table_lookup (pool_data->_criteria_score,
                                         (const void *) fencer->_criteria_quark);

    if (score == 0)
    {
      return TRUE;
    }
    else if (criteria_data->_max_floating_fencers == 0)
    {
      return score < criteria_data->_max_criteria_occurrence;
    }
    else
    {
      return score < criteria_data->_max_criteria_occurrence-1;
    }
  }
  return FALSE;
}

// --------------------------------------------------------------------------------
void SmartSwapper::InsertCriteria (GHashTable   *table,
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
void SmartSwapper::SetExpectedData (GQuark        quark,
                                    CriteriaData *criteria_data,
                                    SmartSwapper *swapper)
{
  criteria_data->_max_criteria_occurrence = criteria_data->_count / swapper->_nb_pools;

  criteria_data->_max_floating_fencers = criteria_data->_count % swapper->_nb_pools;
  if (criteria_data->_max_floating_fencers)
  {
    criteria_data->_max_criteria_occurrence++;
  }

  criteria_data->_floating_fencers = 0;
  criteria_data->_has_errors       = FALSE;

  g_print ("%d x %20s >> %d (max) %d (floating)\n", criteria_data->_count, g_quark_to_string (quark),
           criteria_data->_max_criteria_occurrence, criteria_data->_max_floating_fencers);
}

// --------------------------------------------------------------------------------
guint SmartSwapper::GetPoolIndex (guint fencer_index)
{
  if (((fencer_index / _nb_pools) % 2) == 0)
  {
    return (fencer_index % _nb_pools);
  }
  else
  {
    return (_nb_pools-1 - fencer_index % _nb_pools);
  }
}

// --------------------------------------------------------------------------------
Player *SmartSwapper::GetNextPlayer (Pool *for_pool)
{
  return NULL;
}
