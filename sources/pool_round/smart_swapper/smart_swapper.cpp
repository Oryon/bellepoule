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
    _moved            = 0;

    _criteria_count          = 0;
    _distributions           = NULL;
    _previous_criteria_quark = 0;

    _nb_pools   = 0;
    _pool_table = NULL;

    _has_errors = FALSE;
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
                                                              criteria_id);
        criteria_id->Release ();
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
    ExtractMovables   ();
    DispatchErrors    ();
    DispatchFloatings ();
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
                                    "swapped_from",
                                    (void *) fencer->_original_pool->_id);
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

      player->RemoveData (_owner,
                          "swapped_from");
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
          CriteriaValue *criteria_value;

          if (g_hash_table_lookup_extended (criteria_distribution,
                                            (gconstpointer) quark,
                                            NULL,
                                            (gpointer *) &criteria_value) == FALSE)
          {
            criteria_value = new CriteriaValue (criteria_id, _owner);

            g_hash_table_insert (criteria_distribution,
                                 (gpointer) quark,
                                 criteria_value);
          }
          criteria_value->Use (player);
        }

        g_free (user_image);
      }
    }
  }

  // --------------------------------------------------------------------------------
  GHashTable *SmartSwapper::LookUpDistribution (GSList              *fencer_list,
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
                          (GHFunc) CriteriaValue::Profile,
                          (void *) _nb_pools);

    return criteria_distribution;
  }

  // --------------------------------------------------------------------------------
  void SmartSwapper::ExtractMovables ()
  {
    PRINT (GREEN "\n\nExtraction" ESC);

    for (guint i = 0; i < _nb_pools; i++)
    {
      PoolData       *pool_data = _pool_table[i];
      GHashTableIter  iter;
      gpointer        key;

      g_hash_table_iter_init (&iter,
                              pool_data->_criteria_scores[_criteria_depth]);

      PRINT (BLUE "   #%d" ESC, i+1);
      while (g_hash_table_iter_next (&iter,
                                     &key,
                                     NULL))
      {
        GQuark         quark          = GPOINTER_TO_UINT (key);
        CriteriaValue *criteria_value = (CriteriaValue *) g_hash_table_lookup (_distributions[_criteria_depth],
                                                                               (const void *) quark);

        {
          GList *current = g_list_last (pool_data->_fencer_list);

          while (current)
          {
            Fencer *fencer = (Fencer *) current->data;

            if (   (fencer->_rank != pool_data->_id) // Prevent pool leaders from being moved
                && FencerIsMovable (fencer)
                && (fencer->_criteria_quarks[_criteria_depth] == quark))
            {
              guint teammate_rank = pool_data->GetTeammateRank (fencer, _criteria_depth);

              if (criteria_value->CanFloat (fencer))
              {
                PRINT (GREEN "  << FLOATING >>" ESC " %s\n", fencer->_player->GetName ());
                _floating_list = g_list_prepend (_floating_list,
                                                 fencer);

                pool_data->RemoveFencer ((Fencer *) fencer);
              }
              else if (teammate_rank >= criteria_value->GetErrorLine (fencer))
              {
                PRINT (RED "  << ERROR >>" ESC " %s\n", fencer->_player->GetName ());
                _error_list = g_list_prepend (_error_list,
                                              fencer);

                pool_data->SetError     (_criteria_depth, quark);
                pool_data->RemoveFencer (fencer);
              }
              else
              {
                break;
              }
            }

            current = g_list_previous (current);
          }
        }
      }
    }

    _error_list = g_list_sort (_error_list,
                               (GCompareFunc) CompareFencerRank);
    _floating_list = g_list_sort (_floating_list,
                                  (GCompareFunc) CompareFencerRank);
  }

  // --------------------------------------------------------------------------------
  void SmartSwapper::DispatchErrors ()
  {
    PRINT (GREEN "\n\nDispatch ERRORS" ESC);

    DispatchFencers (_error_list,
                     FALSE);

    g_list_free (_error_list);
    _error_list = NULL;
  }

  // --------------------------------------------------------------------------------
  void SmartSwapper::DispatchFloatings ()
  {
    PRINT (GREEN "\n\nDispatch FLOATINGS" ESC);

    DispatchFencers (_floating_list,
                     TRUE);

    g_list_free (_floating_list);
    _floating_list = NULL;
  }

  // --------------------------------------------------------------------------------
  void SmartSwapper::DispatchFencers (GList    *list,
                                      gboolean  favorite_pool_first)
  {
    if (list == NULL)
    {
      return;
    }

    while (list)
    {
      Fencer *fencer = (Fencer *) list->data;

      fencer->Dump (_owner);

      // Try favorite pool first
      if (favorite_pool_first)
      {
        PoolData *favorite_pool = fencer->_original_pool;

        for (guint profile_type = 0; _pool_profiles.Exists (profile_type); profile_type++)
        {
          if (MoveFencerTo (fencer,
                            favorite_pool,
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

          if (   favorite_pool_first
              || pool_data->HasErrorsFor (_criteria_depth,                                      // Forget pools where over population
                                          fencer->_criteria_quarks[_criteria_depth]) == FALSE)  // errors have been detected the given criteria
          {
            if (MoveFencerTo (fencer,
                              pool_data,
                              _pool_profiles.GetSize (profile_type)))
            {
              goto next_fencer;
            }
          }
        }
      }

      // Moving failed
      _remaining_errors = g_slist_prepend (_remaining_errors,
                                           fencer);
      PRINT (RED "  << %s >>\n" ESC, fencer->_player->GetName ());

next_fencer:
      ChangeFirstPoolTotry ();
      list = g_list_next (list);
    }

    // DumpPools ();
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
      if (FencerCanGoTo (fencer,
                         pool_data,
                         _criteria_depth))
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
                                        PoolData *pool_data,
                                        guint     depth)
  {
    if (depth != 0)
    {
      if (FencerCanGoTo (fencer,
                         pool_data,
                         depth-1) == FALSE)
      {
        return FALSE;
      }
    }

    {
      guint          teammate_rank  = pool_data->GetTeammateRank (fencer, depth);
      CriteriaValue *criteria_value = (CriteriaValue *) g_hash_table_lookup (_distributions[depth],
                                                                             (const void *) fencer->_criteria_quarks[depth]);

      return (teammate_rank < criteria_value->GetErrorLine (fencer));
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
  guint SmartSwapper::GetMoved ()
  {
    return _moved;
  }
}
