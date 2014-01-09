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

#include "util/object.hpp"

#include "pool_round/pool.hpp"
#include "pool_round/pool_zone.hpp"

#include "smart_swapper.hpp"

namespace SmartSwapper
{
#define DEBUG_SWAPPING

#ifdef DEBUG_SWAPPING
#define PRINT(...)\
  {\
    g_print (__VA_ARGS__);\
    g_print ("\n");\
  }
#else
#define PRINT(...)
#endif

  // --------------------------------------------------------------------------------
  SmartSwapper::SmartSwapper (Object *owner)
    : Object ("SmartSwapper")
  {
    _owner = owner;

    _remaining_errors = NULL;
    _error_list       = NULL;
    _floating_list    = NULL;

    _criteria_count          = 0;
    _distributions           = NULL;
    _previous_criteria_quark = 0;

    _nb_pools   = 0;
    _pool_table = NULL;

    _has_errors = FALSE;

    _moved      = 0;
    _over_count = 0;
  }

  // --------------------------------------------------------------------------------
  SmartSwapper::~SmartSwapper ()
  {
    Clean ();
  }

  // --------------------------------------------------------------------------------
  Pool::Swapper *SmartSwapper::Create (Object *owner)
  {
    return new SmartSwapper (owner);
  }

  // --------------------------------------------------------------------------------
  void SmartSwapper::Clean ()
  {
    for (guint i = 0; i < _criteria_count; i++)
    {
      g_hash_table_destroy (_distributions[i]);
    }

    g_free (_distributions);
  }

  // --------------------------------------------------------------------------------
  void SmartSwapper::Delete ()
  {
    Release ();
  }

  // --------------------------------------------------------------------------------
  void SmartSwapper::Init (GSList *zones,
                           guint   fencer_count)
  {
    Clean ();

    _zones    = zones;
    _nb_pools = g_slist_length (zones);

    _pool_profiles.Configure (fencer_count,
                              _nb_pools);
  }

  // --------------------------------------------------------------------------------
  void SmartSwapper::Swap (GSList *criteria_list,
                           GSList *fencer_list)
  {
    _moved          = 0;
    _over_count     = 0;
    _criteria_count = g_slist_length (criteria_list);
    _distributions  = g_new (GHashTable *, _criteria_count);

    CreatePoolTable (_zones);

    {
      GSList *current = fencer_list;

      for (guint i = 0; current != NULL; i++)
      {
        AddPlayerToPool ((Player *) current->data,
                         _pool_table[GetPoolIndex (i)],
                         criteria_list);

        current = g_slist_next (current);
      }
    }

    for (_criteria_depth = 0; _criteria_depth < _criteria_count; _criteria_depth++)
    {
      {
        AttributeDesc       *attr_desc   = (AttributeDesc *) criteria_list->data;
        Player::AttributeId *criteria_id = new Player::AttributeId (attr_desc->_code_name);

        _distributions[_criteria_depth] = LookUpDistribution (fencer_list,
                                                              _criteria_depth,
                                                              criteria_id);
      }

      if (_criteria_depth == 0)
      {
        Iterate ();
      }
      else
      {
        GHashTableIter iter;
        gpointer       key;

        g_hash_table_iter_init (&iter,
                                _distributions[_criteria_depth-1]);

        while (g_hash_table_iter_next (&iter,
                                       &key,
                                       NULL))
        {
          _previous_criteria_quark = GPOINTER_TO_UINT (key);

          PRINT (RED "Movable ==> %s\n" ESC, g_quark_to_string (_previous_criteria_quark));
          Iterate ();
        }
      }

      criteria_list = g_slist_next (criteria_list);
    }

    StoreSwapping ();
    DeletePoolTable ();
  }

  // --------------------------------------------------------------------------------
  void SmartSwapper::Iterate ()
  {
    FindLackOfPopulationErrors  ();
    ExtractOverPopulationErrors ();
    ExtractFloatings            ();

    DispatchErrors    ();
    DispatchFloatings ();

    g_hash_table_destroy (_lack_table);
  }

  // --------------------------------------------------------------------------------
  void SmartSwapper::StoreSwapping ()
  {
    for (guint i = 0; i < _nb_pools; i++)
    {
      PoolData *pool_data = _pool_table[i];
      GList    *current   = pool_data->_fencer_list;

      while (current)
      {
        Fencer *fencer = (Fencer *) current->data;

        if (fencer->_player->GetIntData (_owner, "no_swapping_pool") != (gint) (i+1))
        {
          _moved++;

          fencer->_player->SetData (_owner,
                                    "swap_error",
                                    (void *) 1);
        }

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
    _pool_table = g_new (PoolData *, _nb_pools);

    for (guint i = 0; i < _nb_pools; i++)
    {
      Pool::PoolZone *zone = (Pool::PoolZone *) zones->data;

      _pool_table[i] = new PoolData (zone->GetPool (),
                                     i+1,
                                     &_pool_profiles,
                                     _criteria_count);

      zones = g_slist_next (zones);
    }

    _first_pool_to_try = 0;
  }

  // --------------------------------------------------------------------------------
  void SmartSwapper::DeletePoolTable ()
  {
    for (guint i = 0; i < _nb_pools; i++)
    {
      delete (_pool_table[i]);
    }

    g_free (_pool_table);
    _pool_table = NULL;
  }

  // --------------------------------------------------------------------------------
  void SmartSwapper::AddPlayerToPool (Player   *player,
                                      PoolData *pool_data,
                                      GSList   *criteria_list)
  {
    Player::AttributeId stage_start_rank_attr_id ("stage_start_rank", _owner);
    Attribute *stage_start_rank = player->GetAttribute (&stage_start_rank_attr_id);

    if (stage_start_rank == NULL)
    {
      g_print (RED "===>> E R R O R\n" ESC);
    }
    else
    {
      Fencer *fencer = new Fencer (player,
                                   stage_start_rank->GetUIntValue (),
                                   pool_data,
                                   _criteria_count);

      player->SetData (_owner,
                       "swap_error",
                       0);
      player->SetData (_owner,
                       "no_swapping_pool",
                       (void *) pool_data->_id);

      for (guint i = 0; i < _criteria_count; i++)
      {
        AttributeDesc       *attr_desc     = (AttributeDesc *) criteria_list->data;
        Player::AttributeId *criteria_id   = new Player::AttributeId (attr_desc->_code_name);
        Attribute           *criteria_attr = player->GetAttribute (criteria_id);

        if (criteria_attr)
        {
          gchar *user_image = criteria_attr->GetUserImage (AttributeDesc::LONG_TEXT);

          fencer->_criteria_quarks[i] = g_quark_from_string (user_image);
          g_free (user_image);
        }
        criteria_id->Release ();

        criteria_list = g_slist_next (criteria_list);
      }

      fencer->Dump (_owner);
      fencer->_original_pool->AddFencer (fencer);
    }
  }

  // --------------------------------------------------------------------------------
  void SmartSwapper::InsertCriteria (Player              *player,
                                     GHashTable          *criteria_distribution,
                                     Player::AttributeId *criteria_id)
  {
    if (criteria_id)
    {
      Attribute *criteria_attr = player->GetAttribute (criteria_id);

      if (criteria_attr)
      {
        gchar *user_image = criteria_attr->GetUserImage (AttributeDesc::LONG_TEXT);
        GQuark quark      = g_quark_from_string (user_image);

        {
          Criteria *criteria;

          if (g_hash_table_lookup_extended (criteria_distribution,
                                            (gconstpointer) quark,
                                            NULL,
                                            (gpointer *) &criteria) == FALSE)
          {
            criteria = new Criteria ();

            g_hash_table_insert (criteria_distribution,
                                 (gpointer) quark,
                                 criteria);
          }
          criteria->Use ();
        }

        g_free (user_image);
      }
    }
  }

  // --------------------------------------------------------------------------------
  GHashTable *SmartSwapper::LookUpDistribution (GSList              *fencer_list,
                                                guint                criteria_index,
                                                Player::AttributeId *criteria_id)
  {
    GHashTable *criteria_distribution = g_hash_table_new_full (NULL,
                                                               NULL,
                                                               NULL,
                                                               (GDestroyNotify) Object::TryToRelease);

    while (fencer_list)
    {
      InsertCriteria ((Player *) fencer_list->data,
                      criteria_distribution,
                      criteria_id);

      fencer_list = g_slist_next (fencer_list);
    }

    g_hash_table_foreach (criteria_distribution,
                          (GHFunc) Criteria::Profile,
                          (void *) _nb_pools);

    // Keep the results available until the end of the processing
    for (guint i = 0; i < _nb_pools; i++)
    {
      PoolData       *pool_data = _pool_table[i];
      GHashTableIter  iter;
      gpointer        key;
      gpointer        value;

      g_hash_table_iter_init (&iter,
                              pool_data->_criteria_scores[criteria_index]);
      while (g_hash_table_iter_next (&iter,
                                     &key,
                                     &value))
      {
        g_hash_table_insert (pool_data->_original_criteria_scores[criteria_index],
                             key,
                             value);
      }
    }

    return criteria_distribution;
  }

  // --------------------------------------------------------------------------------
  void SmartSwapper::ExtractOverPopulationErrors ()
  {
    PRINT (GREEN "\n\nExtract overpopulation ERRORS" ESC);

    for (guint i = 0; i < _nb_pools; i++)
    {
      PoolData       *pool_data = _pool_table[i];
      GHashTableIter  iter;
      gpointer        key;
      gpointer        value;

      g_hash_table_iter_init (&iter,
                              pool_data->_criteria_scores[_criteria_depth]);

      PRINT ("   #%d", i+1);
      while (g_hash_table_iter_next (&iter,
                                     &key,
                                     &value))
      {
        GQuark quark = GPOINTER_TO_UINT (key);
        guint  count = GPOINTER_TO_UINT (value);
        Criteria *criteria = (Criteria *) g_hash_table_lookup (_distributions[_criteria_depth],
                                                               (const void *) quark);

        if (criteria && (count > criteria->_max_criteria_count))
        {
          for (guint error = 0; error < count - criteria->_max_criteria_count; error++)
          {
            Fencer *fencer = ExtractFencer (pool_data,
                                            quark,
                                            &_error_list);

            if (fencer)
            {
              _over_count++;
              fencer->_over_population_error = TRUE;
            }
          }
        }
      }
    }

    _error_list = g_list_sort (_error_list,
                               (GCompareFunc) CompareFencerRank);
  }

  // --------------------------------------------------------------------------------
  void SmartSwapper::FindLackOfPopulationErrors ()
  {
    PRINT (GREEN "\n\nFind lack of population ERRORS" ESC);

    GHashTableIter iter;
    gpointer       key;
    gpointer       value;

    _lack_table = g_hash_table_new (NULL,
                                    NULL);

    g_hash_table_iter_init (&iter,
                            _distributions[_criteria_depth]);

    while (g_hash_table_iter_next (&iter,
                                   &key,
                                   &value))
    {
      Criteria *criteria = (Criteria *) value;

      if (   criteria
          && criteria->HasFloatingProfile ()
          && (criteria->_max_criteria_count > 1))
      {
        GQuark quark = GPOINTER_TO_UINT (key);

        for (guint i = 0; i < _nb_pools; i++)
        {
          PoolData *pool_data = _pool_table[i];
          guint     score;

          score = GPOINTER_TO_UINT (g_hash_table_lookup (pool_data->_criteria_scores[_criteria_depth],
                                                         (const void *) quark));

          if (score < criteria->_max_criteria_count-1)
          {
            g_hash_table_insert (_lack_table,
                                 (gpointer) quark,
                                 (gpointer) (criteria->_max_criteria_count-1));
          }
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  void SmartSwapper::ExtractFloatings ()
  {
    PRINT (GREEN "\n\nExtract FLOATINGS" ESC);

    for (guint i = 0; i < _nb_pools; i++)
    {
      PoolData       *pool_data = _pool_table[i];
      GHashTableIter  iter;
      gpointer        key;
      gpointer        value;

      g_hash_table_iter_init (&iter,
                              pool_data->_criteria_scores[_criteria_depth]);

      PRINT ("   #%d", i+1);
      while (g_hash_table_iter_next (&iter,
                                     &key,
                                     &value))
      {
        GQuark quark = GPOINTER_TO_UINT (key);
        guint  count = GPOINTER_TO_UINT (value);

        Criteria *criteria = (Criteria *) g_hash_table_lookup (_distributions[_criteria_depth],
                                                               (const void *) quark);

        if (   (criteria == NULL)
            || (criteria->HasFloatingProfile () && (count == criteria->_max_criteria_count)))
        {
          ExtractFencer (pool_data,
                         quark,
                         &_floating_list);
        }
      }
    }

    _floating_list = g_list_sort (_floating_list,
                                  (GCompareFunc) CompareFencerRank);


    PRINT (GREEN "\n\nTransform LACKS into ERRORS" ESC);
    // Transform lack of population into real errors
    {
      GHashTableIter iter;
      gpointer       key;
      gpointer       value;

      g_hash_table_iter_init (&iter,
                              _lack_table);
      while (g_hash_table_iter_next (&iter,
                                     &key,
                                     &value))
      {
        GList  *current_floating = g_list_last (_floating_list);
        GQuark  quark            = GPOINTER_TO_UINT (key);
        guint   lack_count       = GPOINTER_TO_UINT (value);

        for (guint i = 0; i < lack_count; i++)
        {
          if (current_floating)
          {
            Fencer *fencer = (Fencer *) current_floating->data;

            if (   FencerIsMovable (fencer)
                && (fencer->_criteria_quarks[_criteria_depth] == quark))
            {
              fencer->_over_population_error = TRUE;
              PRINT ("    transform %s\n", fencer->_player->GetName ());
            }
            current_floating = g_list_previous (current_floating);
          }
          else
          {
            break;
          }
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  Fencer *SmartSwapper::ExtractFencer (PoolData  *from_pool,
                                       GQuark     with_criteria,
                                       GList    **to_list)
  {
    GList  *current = g_list_last (from_pool->_fencer_list);
    Fencer *fencer  = (Fencer *) current->data;

    while (   (fencer->_rank == from_pool->_id) // Prevent pool leaders from being moved
           || (FencerIsMovable (fencer) == FALSE)
           || (fencer->_criteria_quarks[_criteria_depth] != with_criteria))
    {
      current = g_list_previous (current);
      if (current == NULL)
      {
        return NULL;
      }
      fencer = (Fencer *) current->data;
    }

    if (to_list)
    {
      *to_list = g_list_prepend (*to_list,
                                 current->data);

      fencer->Dump (_owner);
      from_pool->RemoveFencer ((Fencer *) current->data);
    }

    return fencer;
  }

  // --------------------------------------------------------------------------------
  void SmartSwapper::DispatchErrors ()
  {
    PRINT (GREEN "\n\nDispatch ERRORS" ESC);

    DispatchFencers (_error_list);

    g_list_free (_error_list);
    _error_list = NULL;
  }

  // --------------------------------------------------------------------------------
  void SmartSwapper::DispatchFloatings ()
  {
    GList *failed_list = NULL;

    PRINT (GREEN "\n\nDispatch FLOATINGS" ESC);

    DispatchFencers (_floating_list);

    PRINT (GREEN "\n\nDispatch REMAINING" ESC);
    // Remaining errors
    {
      GSList *current_error = _remaining_errors;

      while (current_error)
      {
        Fencer *error            = (Fencer *) current_error->data;
        GList  *current_floating = g_list_last (_floating_list);

        while (current_floating)
        {
          Fencer *floating = (Fencer *) current_floating->data;

          if (floating->_new_pool)
          {
            if (FencerCanGoTo (error, floating->_new_pool))
            {
              PoolData *new_pool = floating->_new_pool;

              // Try this assignment
              new_pool->AddFencer    (error);
              new_pool->RemoveFencer (floating);

              for (guint profile_type = 0; _pool_profiles.Exists (profile_type); profile_type++)
              {
                for (guint i = 0; i < _nb_pools; i++)
                {
                  PoolData *pool_data = GetPoolToTry (i);

                  if (MoveFencerTo (floating,
                                    pool_data,
                                    _pool_profiles.GetSize (profile_type)))
                  {
                    goto next_error;
                  }
                }
              }

              // Failed! Restore the previous assignment
              new_pool->RemoveFencer (error);
              new_pool->AddFencer    (floating);
            }
          }

          current_floating = g_list_previous (current_floating);
        }

        PRINT ("!!!!> %s", error->_player->GetName ());
        failed_list = g_list_prepend (failed_list,
                                      error);

next_error:
        ChangeFirstPoolTotry ();
        current_error = g_slist_next (current_error);
      }
    }

    // Last chance to find a pool for the errors
    {
      g_slist_free (_remaining_errors);
      _remaining_errors = NULL;

      DispatchFencers (failed_list);
      _has_errors = (failed_list != NULL);
      g_list_free (failed_list);
    }

    g_list_free (_floating_list);
    _floating_list = NULL;
  }

  // --------------------------------------------------------------------------------
  void SmartSwapper::DispatchFencers (GList *list)
  {
    if (list == NULL)
    {
      return;
    }

    while (list)
    {
      Fencer *fencer = (Fencer *) list->data;

      fencer->Dump (_owner);

      // Try original pool first
      if (fencer->_over_population_error == FALSE)
      {
        for (guint profile_type = 0; _pool_profiles.Exists (profile_type); profile_type++)
        {
          if (MoveFencerTo (fencer,
                            fencer->_original_pool,
                            _pool_profiles.GetSize (profile_type)))
          {
            goto next_fencer;
          }
        }
      }

      for (guint profile_type = 0; _pool_profiles.Exists (profile_type); profile_type++)
      {
        for (guint i = 0; i < _nb_pools; i++)
        {
          PoolData *pool_data = GetPoolToTry (i);

          if (MoveFencerTo (fencer,
                            pool_data,
                            _pool_profiles.GetSize (profile_type)))
          {
            goto next_fencer;
          }
        }
      }

      // Moving failed
      _remaining_errors = g_slist_prepend (_remaining_errors,
                                           fencer);
      fencer->_over_population_error = FALSE;

next_fencer:
      ChangeFirstPoolTotry ();
      list = g_list_next (list);
    }

    DumpPools ();
  }

  // --------------------------------------------------------------------------------
  void SmartSwapper::DumpPools ()
  {
#ifdef DEBUG_SWAPPING
    PRINT (" ");
    PRINT (" ");
    for (guint i = 0; i < _nb_pools; i++)
    {
      PoolData *pool_data = _pool_table[i];
      GList    *current   = pool_data->_fencer_list;

      PRINT ("--- Pool #%d", i+1);
      while (current)
      {
        Fencer *fencer = (Fencer *) current->data;

        fencer->Dump (_owner);
        current = g_list_next (current);
      }
    }
#endif
  }

  // --------------------------------------------------------------------------------
  gboolean SmartSwapper::MoveFencerTo (Fencer   *fencer,
                                       PoolData *pool_data,
                                       guint     max_pool_size)
  {
    if (pool_data->_size < max_pool_size)
    {
      // Forget pools where over population errors have been detected
      // for the given criteria
      if (fencer->_over_population_error)
      {
        Criteria *criteria = (Criteria *) g_hash_table_lookup (_distributions[_criteria_depth],
                                                               (const void *) fencer->_criteria_quarks[_criteria_depth]);

        if (criteria)
        {
          guint score = GPOINTER_TO_UINT (g_hash_table_lookup (pool_data->_original_criteria_scores[_criteria_depth],
                                                               (const void *) fencer->_criteria_quarks[_criteria_depth]));

          if (score > criteria->_max_criteria_count)
          {
            return FALSE;
          }
        }
      }

      if (FencerCanGoTo (fencer, pool_data))
      {
        pool_data->AddFencer (fencer);
        PRINT ("          --> #%d (%d)", pool_data->_pool->GetNumber (), g_list_length (pool_data->_fencer_list));
        return TRUE;
      }
    }

    return FALSE;
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

  // --------------------------------------------------------------------------------
  gboolean SmartSwapper::FencerIsMovable (Fencer *fencer)
  {
    return (   (_criteria_depth == 0)
            || (_previous_criteria_quark == fencer->_criteria_quarks[_criteria_depth-1]));
  }

  // --------------------------------------------------------------------------------
  gboolean SmartSwapper::FencerCanGoTo (Fencer   *fencer,
                                        PoolData *pool_data)
#if 0
  {
    for (guint i = 0; i < _criteria_depth; i++)
    {
      Criteria *criteria = (Criteria *) g_hash_table_lookup (_distributions[i],
                                                             (const void *) fencer->_criteria_quarks[i]);

      if (criteria != NULL)
      {
        guint score = GPOINTER_TO_UINT (g_hash_table_lookup (pool_data->_criteria_scores[i],
                                                             (const void *) fencer->_criteria_quarks[i]));

        if (score && fencer->_over_population_error)
        {
          if (   criteria->HasFloatingProfile ()
              && (score >= criteria->_max_criteria_count - 1))
          {
            return FALSE;
          }
        }

        if (score >= criteria->_max_criteria_count)
        {
          return FALSE;
        }
      }
    }

    return TRUE;
  }
#endif
  {
    Criteria *criteria = (Criteria *) g_hash_table_lookup (_distributions[_criteria_depth],
                                                           (const void *) fencer->_criteria_quarks[_criteria_depth]);

    if (criteria == NULL)
    {
      return TRUE;
    }

    {
      guint score = GPOINTER_TO_UINT (g_hash_table_lookup (pool_data->_criteria_scores[_criteria_depth],
                                                           (const void *) fencer->_criteria_quarks[_criteria_depth]));

      if (score && fencer->_over_population_error)
      {
        if (criteria->HasFloatingProfile ())
        {
          return score < criteria->_max_criteria_count - 1;
        }
      }

      return score < criteria->_max_criteria_count;
    }
  }

  // --------------------------------------------------------------------------------
  guint SmartSwapper::HasErrors ()
  {
    return _has_errors;
  }

  // --------------------------------------------------------------------------------
  PoolData *SmartSwapper::GetPoolToTry (guint index)
  {
    if (index + _first_pool_to_try < _nb_pools)
    {
      return _pool_table[index + _first_pool_to_try];
    }
    else
    {
      return _pool_table[index + _first_pool_to_try - _nb_pools];
    }
  }

  // --------------------------------------------------------------------------------
  void SmartSwapper::ChangeFirstPoolTotry ()
  {
    _first_pool_to_try++;
    if (_first_pool_to_try >= _nb_pools)
    {
      _first_pool_to_try = 0;
    }
  }

  // --------------------------------------------------------------------------------
  guint SmartSwapper::GetOverCount ()
  {
    return _over_count;
  }

  // --------------------------------------------------------------------------------
  guint SmartSwapper::GetMoved ()
  {
    return _moved;
  }
}
