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
#include "util/attribute.hpp"
#include "util/attribute_desc.hpp"
#include "util/player.hpp"
#include "../pool.hpp"
#include "../pool_zone.hpp"
#include "fencer_proxy.hpp"
#include "pool_proxy.hpp"
#include "criteria.hpp"
#include "list_crawler.hpp"

#include "neo_swapper.hpp"

namespace NeoSwapper
{
#ifdef DEBUG
#define DEBUG_SWAPPING
#endif

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
  Swapper::Swapper (Object *owner)
    : Object ("NeoSwapper")
  {
    _owner = owner;

    _error_list = NULL;

    _criteria_list  = NULL;
    _criteria_count = 0;
    _distributions  = NULL;

    _nb_pools    = 0;
    _pool_table  = NULL;
    _fencer_list = NULL;
  }

  // --------------------------------------------------------------------------------
  Swapper::~Swapper ()
  {
    Clean ();
  }

  // --------------------------------------------------------------------------------
  Pool::Swapper *Swapper::Create (Object *owner)
  {
    return new Swapper (owner);
  }

  // --------------------------------------------------------------------------------
  void Swapper::Delete ()
  {
    Release ();
  }

  // --------------------------------------------------------------------------------
  void Swapper::Clean ()
  {
    DeletePoolTable ();

    g_list_free (_error_list);
    _error_list = NULL;

    for (guint i = 0; i < _criteria_count; i++)
    {
      g_hash_table_destroy (_distributions[i]);
    }
    _criteria_count = 0;

    g_free (_distributions);
    _distributions = NULL;
  }

  // --------------------------------------------------------------------------------
  void Swapper::Configure (GSList *zones,
                           GSList *criteria_list)
  {
    Clean ();

    {
      _zones    = zones;
      _nb_pools = g_slist_length (zones);
    }

    {
      _criteria_list  = criteria_list;
      _criteria_count = g_slist_length (criteria_list);
      _distributions  = g_new (GHashTable *, _criteria_count);
      for (guint i = 0; i < _criteria_count; i++)
      {
        _distributions[i] = g_hash_table_new_full (NULL,
                                                   NULL,
                                                   NULL,
                                                   (GDestroyNotify) Criteria::Delete);
      }
    }

    CreatePoolTable (_zones);
  }

  // --------------------------------------------------------------------------------
  void Swapper::MoveFencer (Player         *player,
                            Pool::PoolZone *from_pool_zone,
                            Pool::PoolZone *to_pool_zone)
  {
    if (_distributions)
    {
      if (from_pool_zone)
      {
        Pool::Pool  *from_pool = from_pool_zone->GetPool ();
        PoolProxy   *from      = _pool_table[from_pool->GetNumber () - 1];
        FencerProxy *fencer    = from->GetFencerProxy (player);

        from->RemoveFencer (fencer);

        if (to_pool_zone)
        {
          Pool::Pool *to_pool = to_pool_zone->GetPool ();
          PoolProxy  *to      = _pool_table[to_pool->GetNumber () - 1];

          to->InsertFencer (fencer);
        }
      }

      FindErrors (_criteria_count);
    }
  }

  // --------------------------------------------------------------------------------
  void Swapper::CheckCurrentDistribution ()
  {
    for (guint i = 0; i < _nb_pools; i++)
    {
      PoolProxy  *pool_proxy = _pool_table[i];
      Pool::Pool *pool       = pool_proxy->_pool;
      GSList     *current    = pool->GetFencerList ();

      while (current)
      {
        ManageFencer ((Player *) current->data,
                      pool_proxy);

        current = g_slist_next (current);
      }
    }

#ifdef DEBUG_SWAPPING
    for (guint i = 0; i < _criteria_count; i++)
    {
      g_hash_table_foreach (_distributions[i],
                            (GHFunc) Criteria::Dump,
                            NULL);
      PRINT (" ");
    }
#endif

    FindErrors (_criteria_count);

    g_list_free (_fencer_list);
    _fencer_list = NULL;
  }

  // --------------------------------------------------------------------------------
  void Swapper::Swap (GSList *fencer_list)
  {
    // Snake seeding
    {
      GSList *current = fencer_list;

      for (guint i = 0; current != NULL; i++)
      {
        guint pool_index;

        if (((i / _nb_pools) % 2) == 0)
        {
          pool_index = i % _nb_pools;
        }
        else
        {
          pool_index = _nb_pools-1 - i % _nb_pools;
        }

        ManageFencer ((Player *) current->data,
                      _pool_table[pool_index]);

        current = g_slist_next (current);
      }
    }

#ifdef DEBUG_SWAPPING
    for (guint i = 0; i < _criteria_count; i++)
    {
      g_hash_table_foreach (_distributions[i],
                            (GHFunc) Criteria::Dump,
                            NULL);
      PRINT (" ");
    }
#endif

    for (guint trial = 0; trial < 2; trial++)
    {
      // Error processing
      for (guint depth = 1; depth <= _criteria_count; depth++)
      {
        FindErrors (depth);
        DispatchErrors (depth);
      }

      // Dispatch remaining errors without swapping
      for (GList *current = _error_list; current; current = g_list_next (current))
      {
        FencerProxy *error      = (FencerProxy *) current->data;
        PoolProxy   *error_pool = error->_new_pool;

        error_pool->RemoveFencer (error);
        for (guint i = 0; i < _nb_pools; i++)
        {
          PoolProxy *pool_proxy = _pool_table[i];

          if (FencerCanGoTo (error,
                             pool_proxy,
                             _criteria_count-1))
          {
            pool_proxy->InsertFencer (error);
            break;
          }

          if (i == (_nb_pools - 1))
          {
            error_pool->InsertFencer (error);
          }
        }
      }
      FindErrors (_criteria_count);

      // Ultimate chance to fix the remaining errors
      if (_error_list)
      {
        // Break the current layout by forcing a fencer to move
        DispatchFencer ((FencerProxy *) _error_list->data,
                        _criteria_count,
                        TRUE);
      }
    }

    FindErrors (_criteria_count);
    StoreSwapping ();

    g_list_free (_fencer_list);
    _fencer_list = NULL;
  }

  // --------------------------------------------------------------------------------
  void Swapper::StoreSwapping ()
  {
    for (guint i = 0; i < _nb_pools; i++)
    {
      PoolProxy *pool_proxy = _pool_table[i];
      GList     *current    = pool_proxy->_fencer_list;

      while (current)
      {
        FencerProxy *fencer = (FencerProxy *) current->data;

#ifdef DEBUG
        if (fencer->_original_pool && (fencer->_original_pool->_id != (i+1)))
        {
          fencer->_player->SetData (_owner,
                                    "swapped_from",
                                    (void *) fencer->_original_pool->_id);
        }
#endif

        fencer->_player->SetData (_owner,
                                  "original_pool",
                                  (void *) (i+1));
        pool_proxy->_pool->AddFencer (fencer->_player,
                                      _owner);

        current = g_list_next (current);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Swapper::CreatePoolTable (GSList *zones)
  {
    _pool_table = g_new (PoolProxy *, _nb_pools);

    for (guint i = 0; i < _nb_pools; i++)
    {
      Pool::PoolZone *zone = (Pool::PoolZone *) zones->data;

      _pool_table[i] = new PoolProxy (zone->GetPool (),
                                      i+1,
                                      _criteria_count);

      zones = g_slist_next (zones);
    }
  }

  // --------------------------------------------------------------------------------
  void Swapper::DeletePoolTable ()
  {
    if (_pool_table)
    {
      for (guint i = 0; i < _nb_pools; i++)
      {
        _pool_table[i]->Release ();
      }

      g_free (_pool_table);
      _pool_table = NULL;
    }
  }

  // --------------------------------------------------------------------------------
  void Swapper::InjectFencer (Player *fencer,
                              guint   pool_id)
  {
    FencerProxy *shadow_fencer = ManageFencer (fencer,
                                               NULL);

    if (pool_id)
    {
      PoolProxy *pool = _pool_table[pool_id-1];

      pool->InsertFencer (shadow_fencer);
    }
    else
    {
      GList *pools_by_size = NULL;

      for (guint i = 0; i < _nb_pools; i++)
      {
        pools_by_size = g_list_insert_sorted (pools_by_size,
                                              _pool_table[i],
                                              GCompareFunc (PoolProxy::CompareSize));
      }

      for (GList *current = pools_by_size; current; current = g_list_next (current))
      {
        PoolProxy *pool = (PoolProxy *) current->data;

        if (FencerCanGoTo (shadow_fencer,
                           pool,
                           _criteria_count))
        {
          PoolProxy *smallest = (PoolProxy *) pools_by_size->data;

          if (pool->_size == smallest->_size)
          {
            pool->InsertFencer (shadow_fencer);
          }
          else
          {
            smallest->InsertFencer (shadow_fencer);
          }
          break;
        }
      }

      g_list_free (pools_by_size);
    }

    StoreSwapping ();
  }

  // --------------------------------------------------------------------------------
  FencerProxy *Swapper::ManageFencer (Player    *player,
                                      PoolProxy *pool_proxy)
  {
    Player::AttributeId  stage_start_rank_attr_id ("stage_start_rank", _owner);
    Attribute           *stage_start_rank = player->GetAttribute (&stage_start_rank_attr_id);

    if (stage_start_rank == NULL)
    {
#ifdef DEBUG
      g_error ("Swapper: %s no start rank !!!", player->GetName ());
#else
      g_warning ("Swapper: %s no start rank !!!", player->GetName ());
#endif
    }

    {
      FencerProxy *fencer = new FencerProxy (player,
                                             stage_start_rank->GetUIntValue (),
                                             pool_proxy,
                                             _criteria_count);

      _fencer_list = g_list_append (_fencer_list,
                                    fencer);

#ifdef DEBUG
      player->RemoveData (_owner,
                          "swapped_from");
#endif

      for (guint i = 0; i < _criteria_count; i++)
      {
        AttributeDesc       *attr_desc   = (AttributeDesc *) g_slist_nth_data (_criteria_list, i);
        Player::AttributeId *criteria_id = new Player::AttributeId (attr_desc->_code_name);

        InsertCriteria (fencer,
                        _distributions[i],
                        criteria_id,
                        i);

        criteria_id->Release ();
      }

      if (fencer->_original_pool)
      {
        fencer->_original_pool->AddFencer (fencer);
      }

      return fencer;
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  void Swapper::InsertCriteria (FencerProxy         *fencer,
                                GHashTable          *criteria_distribution,
                                Player::AttributeId *criteria_id,
                                guint                depth)
  {
    if (criteria_id)
    {
      Player    *player        = fencer->_player;
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
            criteria = new Criteria (quark,
                                     _nb_pools,
                                     depth,
                                     _owner);
            g_hash_table_insert (criteria_distribution,
                                 (gpointer) quark,
                                 criteria);
          }

          fencer->SetCriteria(depth,
                              criteria);
          criteria->Use (fencer);
        }

        g_free (user_image);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Swapper::FindErrors (guint depth)
  {
    PRINT (GREEN "\n\nFind ERRORS" ESC);

    g_list_free (_error_list);
    _error_list = NULL;

    for (guint i = 0; i < depth; i++)
    {
      GHashTableIter  iter;
      gpointer        key;
      Criteria       *criteria;

      g_hash_table_iter_init (&iter,
                              _distributions[i]);

      while (g_hash_table_iter_next (&iter,
                                     &key,
                                     (void **) &criteria))
      {
        PRINT (CYAN "%s" ESC, g_quark_to_string (GPOINTER_TO_UINT (key)));
        _error_list = criteria->GetErrors (_error_list);
      }
    }

    _error_list = g_list_sort (_error_list,
                               (GCompareFunc) FencerProxy::CompareRank);

#ifdef DEBUG_SWAPPING
    {
      GList *current = _error_list;

      PRINT ("\n");
      while (current)
      {
        FencerProxy *fencer = (FencerProxy *) current->data;

        PRINT ("<<  %s >>", fencer->_player->GetName ());
        current = g_list_next (current);
      }
    }
#endif
  }

  // --------------------------------------------------------------------------------
  void Swapper::DispatchErrors (guint depth)
  {
    PRINT (GREEN "\n\nDispatch ERRORS" ESC);

    {
      GList *error = _error_list;

      while (error)
      {
        if (DispatchFencer ((FencerProxy *) error->data,
                            depth))
        {
          FindErrors (depth);
          error = _error_list;
        }
        else
        {
          error = g_list_next (error);
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Swapper::DispatchFencer (FencerProxy *error,
                                    guint        depth,
                                    gboolean     force)
  {
    ListCrawler *crawler    = new ListCrawler (_fencer_list);
    PoolProxy   *error_pool = error->_new_pool;
    GList       *next;

    PRINT ("  %s (%s)", error->_player->GetName (), Criteria::GetImage (error->GetCriteria (depth-1)));
    error_pool->RemoveFencer (error);

    crawler->Reset (error->_rank-1);
    next = crawler->GetNext ();
    while (next)
    {
      FencerProxy *x_fencer = (FencerProxy *) next->data;
      PoolProxy   *x_pool   = x_fencer->_new_pool;

      if (   (error_pool != x_pool)
          && (error->HasSameCriteriaThan (x_fencer, depth-1) == FALSE)
          && (x_fencer->_rank != x_pool->_id)) // Prevent pool leader from being moved
      {
        x_pool->RemoveFencer (x_fencer);

        if (FencerCanGoTo (error,
                           x_pool,
                           depth-1))
        {
          if (   force
              || FencerCanGoTo (x_fencer,
                                error_pool,
                                depth-1))
          {
            PRINT (BLUE "     %s (%s)" ESC, x_fencer->_player->GetName (), Criteria::GetImage (x_fencer->GetCriteria (depth-1)));
            error_pool->InsertFencer (x_fencer);
            x_pool->InsertFencer     (error);

            error = NULL;
            break;
          }
          PRINT (RED "     %s (%s)" ESC, x_fencer->_player->GetName (), Criteria::GetImage (x_fencer->GetCriteria (depth-1)));
        }
        else
        {
          PRINT (RED "   %s (%s)" ESC, x_fencer->_player->GetName (), Criteria::GetImage (x_fencer->GetCriteria (depth-1)));
        }

        x_pool->InsertFencer (x_fencer);
      }
      next = crawler->GetNext ();
    }

    if (error)
    {
      error_pool->InsertFencer (error);
    }

    crawler->Release ();

    return error == NULL;
  }

  // --------------------------------------------------------------------------------
  gboolean Swapper::FencerCanGoTo (FencerProxy *fencer,
                                   PoolProxy   *pool_proxy,
                                   guint        depth)
  {
    if (depth != 0)
    {
      if (FencerCanGoTo (fencer,
                         pool_proxy,
                         depth-1) == FALSE)
      {
        return FALSE;
      }
    }

    {
      Criteria *criteria = fencer->GetCriteria (depth);

      if (criteria == NULL)
      {
        return TRUE;
      }

      return criteria->FreePlaceIn (pool_proxy);
    }
  }

  // --------------------------------------------------------------------------------
  void Swapper::DumpPools ()
  {
#ifdef DEBUG_SWAPPING
    PRINT (" ");
    for (guint i = 0; i < _nb_pools; i++)
    {
      PoolProxy *pool_proxy = _pool_table[i];
      GList     *current    = pool_proxy->_fencer_list;

      PRINT ("--- Pool #%d", i+1);
      while (current)
      {
        FencerProxy *fencer = (FencerProxy *) current->data;

        PRINT ("        %s", fencer->_player->GetName ());
        current = g_list_next (current);
      }
    }
    PRINT (" ");
#endif
  }

  // --------------------------------------------------------------------------------
  guint Swapper::HasErrors ()
  {
#ifdef DEBUG_SWAPPING
    GList *current = _error_list;

    while (current)
    {
      FencerProxy *error = (FencerProxy *) current->data;

      PRINT ("** %s **\n", error->_player->GetName ());

      current = g_list_next (current);
    }
#endif

    return _error_list != NULL;
  }

  // --------------------------------------------------------------------------------
  gboolean Swapper::IsAnError (Player *fencer)
  {
    GList *current = _error_list;

    while (current)
    {
      FencerProxy *error = (FencerProxy *) current->data;

      if (error->_player == fencer)
      {
        return TRUE;
      }
      current = g_list_next (current);
    }
    return FALSE;
  }

  // --------------------------------------------------------------------------------
  guint Swapper::GetMoved ()
  {
    return g_list_length (_error_list);
  }
}
