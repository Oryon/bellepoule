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
// --------------------------------------------------------------------------------
void SmartSwapper::Fencer::Dump (Object *owner)
{
  Player::AttributeId  previous_rank_attr_id ("previous_stage_rank", owner);
  Attribute *previous_stage_rank = _player->GetAttribute (&previous_rank_attr_id);

  g_print ("%d %20ss >> %s (pool #%d)\n",
           previous_stage_rank->GetUIntValue (),
           _player->GetName (),
           g_quark_to_string (_criteria_quark),
           _original_pool+1);
}

// --------------------------------------------------------------------------------
void SmartSwapper::PoolData::ChangeCriteriaScore (GQuark criteria,
                                                  gint   delta_score)
{
  guint score = (guint) g_hash_table_lookup (_criteria_score,
                                             (const void *) criteria);
  score += delta_score;

  g_hash_table_insert (_criteria_score,
                       (void *) criteria,
                       (void *) score);
}

// --------------------------------------------------------------------------------
void SmartSwapper::PoolData::AddFencer (Fencer *fencer)
{
  _fencer_list = g_list_append (_fencer_list,
                                fencer);
  _pool_sizes->NewSize (_size, _size+1);
  _size++;
}

// --------------------------------------------------------------------------------
void SmartSwapper::PoolData::RemoveFencer (GList *fencer)
{
  _fencer_list = g_list_remove_link (_fencer_list,
                                     fencer);
  _pool_sizes->NewSize (_size, _size-1);
  _size--;
}

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
void SmartSwapper::PoolSizes::Configure (guint nb_fencer,
                                         guint nb_pool)
{
  _min_size       = nb_fencer / nb_pool;
  _max_size       = 0;
  _nb_max_reached = 0;
  _nb_max         = nb_fencer % nb_pool;
  if (_nb_max)
  {
    _max_size = _min_size+1;
  }

  _available_sizes[MIN_SIZE] = _min_size;
  _available_sizes[MAX_SIZE] = _max_size;
  _available_sizes[END_MARK] = 0;
}

// --------------------------------------------------------------------------------
void SmartSwapper::PoolSizes::NewSize (guint old_size,
                                       guint new_size)
{
  if (old_size < new_size)
  {
    if (new_size == _max_size)
    {
      _nb_max_reached++;
    }
  }
  else if (old_size > new_size)
  {
    if (old_size == _max_size)
    {
      _nb_max_reached--;
    }
  }

  if (_nb_max_reached >= _nb_max)
  {
    _available_sizes[MAX_SIZE] = 0;
  }
  else
  {
    _available_sizes[MAX_SIZE] = _max_size;
  }
}

// --------------------------------------------------------------------------------
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

  _pool_sizes.Configure (g_slist_length (fencer_list),
                         _nb_pools);

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

      fencer->_player->SetData (_owner,
                               "original_pool",
                               (void *) (i+1));
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
      _pool_table[i]._size           = 0;
      _pool_table[i]._pool_sizes     = &_pool_sizes;
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
      fencer->_rank           = i;
      fencer->_criteria_quark = quark;
      fencer->_is_an_error    = FALSE;
      fencer->_original_pool  = GetPoolIndex (i);

      // criteria score for the current pool
      {
        PoolData *pool_data = &_pool_table[fencer->_original_pool];

        fencer->Dump (_owner);
        pool_data->AddFencer (fencer);

        pool_data->ChangeCriteriaScore (fencer->_criteria_quark,
                                        1);
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

  g_print ("\n\nExtract ERRORS\n");

  for (guint i = 0; i < _nb_pools; i++)
  {
    PoolData *pool_data = &_pool_table[i];

    {
      g_hash_table_iter_init (&iter,
                              pool_data->_criteria_score);

      g_print ("   #%d\n", i+1);
      while (g_hash_table_iter_next (&iter,
                                     (gpointer *) &quark,
                                     (gpointer *) &count))
      {
        CriteriaData *criteria_data = (CriteriaData *) g_hash_table_lookup (_criteria_distribution,
                                                                            (const void *) quark);

        if (criteria_data && (count > criteria_data->_max_criteria_occurrence))
        {
          for (guint error = 0; error < count - criteria_data->_max_criteria_occurrence; error++)
          {
            Fencer *fencer = ExtractFencer (pool_data,
                                            quark,
                                            &_error_list);

            if (fencer)
            {
              fencer->_is_an_error = TRUE;
            }
          }
        }
      }
    }
  }
}

// --------------------------------------------------------------------------------
void SmartSwapper::ExtractFloatings ()
{
  g_print ("\n\nExtract FLOATINGS\n");

  for (guint i = 0; i < _nb_pools; i++)
  {
    PoolData       *pool_data = &_pool_table[i];
    GHashTableIter  iter;
    GQuark          quark;
    guint           count;

    g_hash_table_iter_init (&iter,
                            pool_data->_criteria_score);

    g_print ("   #%d\n", i+1);
    while (g_hash_table_iter_next (&iter,
                                   (gpointer *) &quark,
                                   (gpointer *) &count))
    {
      CriteriaData *criteria_data = (CriteriaData *) g_hash_table_lookup (_criteria_distribution,
                                                                          (const void *) quark);

      if (   (criteria_data == NULL)
          || (criteria_data->_max_floating_fencers && (count == criteria_data->_max_criteria_occurrence)))
      {
        ExtractFencer (pool_data,
                       quark,
                       &_floating_list);
      }
    }
  }
}

// --------------------------------------------------------------------------------
SmartSwapper::Fencer *SmartSwapper::ExtractFencer (PoolData  *from_pool,
                                                   GQuark     with_criteria,
                                                   GSList   **to_list)
{
  Fencer *fencer = NULL;

  {
    GList *current = g_list_last (from_pool->_fencer_list);

    fencer = (Fencer *) current->data;
    while (fencer->_criteria_quark != with_criteria)
    {
      current = g_list_previous (current);
      if (current == NULL)
      {
        return NULL;
      }
      fencer = (Fencer *) current->data;
    }

    *to_list = g_slist_prepend (*to_list,
                                current->data);

    fencer->Dump (_owner);
    from_pool->RemoveFencer (current);

    current = g_list_last (from_pool->_fencer_list);
  }

  from_pool->ChangeCriteriaScore (with_criteria,
                                  -1);

  return fencer;
}

// --------------------------------------------------------------------------------
void SmartSwapper::DispatchErrors ()
{
  g_print ("\n\nDispatch ERRORS\n");

  _error_list = g_slist_sort (_error_list,
                              (GCompareFunc) CompareFencerRank);

  DispatchFencers (_error_list);

  g_slist_free (_error_list);
  _error_list = NULL;
}

// --------------------------------------------------------------------------------
void SmartSwapper::DispatchFloatings ()
{
  g_print ("\n\nDispatch FLOATINGS\n");

  _floating_list = g_slist_sort (_floating_list,
                                 (GCompareFunc) CompareFencerRank);

  DispatchFencers (_floating_list);

  g_slist_free (_floating_list);
  _floating_list = NULL;
}

// --------------------------------------------------------------------------------
void SmartSwapper::DispatchFencers (GSList *list)
{
  while (list)
  {
    Fencer       *fencer        = (Fencer *) list->data;
    CriteriaData *criteria_data = (CriteriaData *) g_hash_table_lookup (_criteria_distribution,
                                                                        (const void *) fencer->_criteria_quark);

    fencer->Dump (_owner);

    // Try original pool first
    if (fencer->_is_an_error == FALSE)
    {
      for (guint size_type = 0; _pool_sizes._available_sizes[size_type] != 0; size_type++)
      {
        PoolData *pool_data = &_pool_table[fencer->_original_pool];

        if (MoveFencerTo (fencer,
                          pool_data,
                          criteria_data,
                          _pool_sizes._available_sizes[size_type]))
        {
          goto next_fencer;
        }
      }
    }

    for (guint size_type = 0; _pool_sizes._available_sizes[size_type] != 0; size_type++)
    {
      for (guint i = 0; i < _nb_pools; i++)
      {
        PoolData *pool_data = &_pool_table[i];

        if (MoveFencerTo (fencer,
                          pool_data,
                          criteria_data,
                          _pool_sizes._available_sizes[size_type]))
        {
          goto next_fencer;
        }
      }
    }

    // Moving failed
    {
      if (fencer->_is_an_error)
      {
        _floating_list = g_slist_prepend (_floating_list,
                                          fencer);
        fencer->_is_an_error = FALSE;
      }
      else
      {
        g_print ("          ERROR\n");
        fencer->Dump (_owner);
      }
    }

next_fencer:
    list = g_slist_next (list);
  }
}

// --------------------------------------------------------------------------------
gboolean SmartSwapper::MoveFencerTo (Fencer       *fencer,
                                     PoolData     *pool_data,
                                     CriteriaData *criteria_data,
                                     guint         max_pool_size)
{
  gboolean moving_allowed = FALSE;

  if (pool_data->_size < max_pool_size)
  {
    if (criteria_data)
    {
      guint score;

      score = (guint) g_hash_table_lookup (pool_data->_criteria_score,
                                           (const void *) fencer->_criteria_quark);

      moving_allowed = score < criteria_data->_max_criteria_occurrence;
      if (score && fencer->_is_an_error)
      {
        if (criteria_data->_max_floating_fencers)
        {
          moving_allowed = score < criteria_data->_max_criteria_occurrence - 1;
        }
      }
    }
    else
    {
      moving_allowed = TRUE;
    }
  }

  if (moving_allowed)
  {
    pool_data->ChangeCriteriaScore (fencer->_criteria_quark,
                                    1);

    pool_data->AddFencer (fencer);
    g_print ("          %d (%d)\n", pool_data->_pool->GetNumber (), g_list_length (pool_data->_fencer_list));
  }

  return moving_allowed;
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
gint SmartSwapper::CompareFencerRank (Fencer *a,
                                      Fencer *b)
{
  return a->_rank - b->_rank;
}
