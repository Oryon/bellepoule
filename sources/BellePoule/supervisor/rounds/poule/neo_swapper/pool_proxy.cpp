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

#include "fencer_proxy.hpp"
#include "criteria.hpp"
#include "pool_proxy.hpp"

namespace NeoSwapper
{
  // --------------------------------------------------------------------------------
  PoolProxy::PoolProxy (Pool::Pool    *pool,
                        guint          id,
                        guint          criteria_count)
    : Object ("SmartSwapper::PoolProxy")
  {
    _pool           = pool;
    _id             = id;
    _criteria_count = criteria_count;

    {
      _fencer_list = NULL;
      _size        = 0;
    }
  }

  // --------------------------------------------------------------------------------
  PoolProxy::~PoolProxy ()
  {
    g_list_foreach (_fencer_list,
                    (GFunc) Object::TryToRelease,
                    NULL);
    g_list_free (_fencer_list);
  }

  // --------------------------------------------------------------------------------
  void PoolProxy::AddFencer (FencerProxy *fencer)
  {
    fencer->_new_pool = this;

    _fencer_list = g_list_append (_fencer_list,
                                  fencer);

    _size++;

    for (guint i = 0; i < _criteria_count; i++)
    {
      Criteria *criteria = fencer->GetCriteria (i);

      criteria->ChangeScore (1,
                             this);
    }
  }

  // --------------------------------------------------------------------------------
  void PoolProxy::InsertFencer (FencerProxy *fencer)
  {
    AddFencer (fencer);

    {
      Player::AttributeId attr_id ("stage_start_rank", _pool->GetDataOwner ());

      _fencer_list = g_list_sort_with_data (_fencer_list,
                                            (GCompareDataFunc) FencerProxy::CompareRank,
                                            &attr_id);
    }
  }

  // --------------------------------------------------------------------------------
  FencerProxy *PoolProxy::GetFencerProxy (Player *fencer)
  {
    GList *current = _fencer_list;

    while (current)
    {
      FencerProxy *proxy = (FencerProxy *) current->data;

      if (proxy->_player == fencer)
      {
        return proxy;
      }

      current = g_list_next (current);
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  void PoolProxy::RemoveFencer (FencerProxy *fencer)
  {
    fencer->_new_pool = NULL;

    _fencer_list = g_list_remove (_fencer_list,
                                  fencer);
    _size--;

    for (guint i = 0; i < _criteria_count; i++)
    {
      Criteria *criteria = fencer->GetCriteria (i);

      criteria->ChangeScore (-1,
                             this);
    }
  }

  // --------------------------------------------------------------------------------
  GList *PoolProxy::GetFencers (guint  from,
                                guint  criteria_depth,
                                GQuark criteria_quark)
  {
    GList *errors  = NULL;
    guint  found   = 0;
    GList *current = _fencer_list;

    while (current)
    {
      FencerProxy *fencer = (FencerProxy *) current->data;

      if (fencer->HasQuark (criteria_depth,
                            criteria_quark))
      {
        if (found >= from)
        {
          errors = g_list_prepend (errors,
                                   fencer);
        }
        found++;
      }

      current = g_list_next (current);
    }

    return errors;
  }

  // --------------------------------------------------------------------------------
  FencerProxy *PoolProxy::GetFencer (guint  at,
                                     guint  criteria_depth,
                                     GQuark criteria_quark)
  {
    guint  found   = 0;
    GList *current = _fencer_list;

    while (current)
    {
      FencerProxy *fencer = (FencerProxy *) current->data;

      if (fencer->HasQuark (criteria_depth,
                            criteria_quark))
      {
        found++;
        if (found >= at)
        {
          return fencer;
        }
      }

      current = g_list_next (current);
    }

    return NULL;
  }
}
