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

#include "pool_fencer.hpp"
#include "pool_data.hpp"

namespace SmartSwapper
{
  // --------------------------------------------------------------------------------
  PoolData::PoolData (Pool::Pool    *pool,
                      guint          id,
                      PoolProfiles  *profiles,
                      guint          criteria_count)
  {
    _pool           = pool;
    _id             = id;
    _pool_profiles  = profiles;
    _criteria_count = criteria_count;

    {
      _fencer_list = NULL;
      _size        = 0;
    }

    {
      _criteria_scores          = g_new (GHashTable *, criteria_count);
      _original_criteria_scores = g_new (GHashTable *, criteria_count);

      for (guint i = 0; i < criteria_count; i++)
      {
        _criteria_scores[i]          = g_hash_table_new (NULL, NULL);
        _original_criteria_scores[i] = g_hash_table_new (NULL, NULL);
      }
    }
  }

  // --------------------------------------------------------------------------------
  PoolData::~PoolData ()
  {
    {
      GList *current = _fencer_list;

      while (current)
      {
        Fencer *fencer = (Fencer *) current->data;

        delete (fencer);
        current = g_list_next (current);
      }

      g_list_free (_fencer_list);
    }

    {
      for (guint i = 0; i < _criteria_count; i++)
      {
        g_hash_table_destroy (_criteria_scores[i]);
        g_hash_table_destroy (_original_criteria_scores[i]);
      }

      g_free (_criteria_scores);
      g_free (_original_criteria_scores);
    }
  }

  // --------------------------------------------------------------------------------
  void PoolData::AddFencer (Fencer *fencer)
  {
    fencer->_new_pool = this;

    _fencer_list = g_list_append (_fencer_list,
                                  fencer);
    _pool_profiles->ChangeFencerCount (_size, _size+1);
    _size++;

    for (guint i = 0; i < _criteria_count; i++)
    {
      ChangeCriteriaScore (fencer->_criteria_quarks[i],
                           _criteria_scores[i],
                           1);
    }
  }

  // --------------------------------------------------------------------------------
  void PoolData::RemoveFencer (Fencer *fencer)
  {
    fencer->_new_pool = NULL;

    _fencer_list = g_list_remove (_fencer_list,
                                  fencer);
    _pool_profiles->ChangeFencerCount (_size, _size-1);
    _size--;

    for (guint i = 0; i < _criteria_count; i++)
    {
      ChangeCriteriaScore (fencer->_criteria_quarks[i],
                           _criteria_scores[i],
                           -1);
    }
  }

  // --------------------------------------------------------------------------------
  void PoolData::ChangeCriteriaScore (GQuark      criteria,
                                      GHashTable *criteria_score,
                                      gint        delta_score)
  {
    guint score = GPOINTER_TO_UINT (g_hash_table_lookup (criteria_score,
                                                         (const void *) criteria));
    score += delta_score;

    g_hash_table_insert (criteria_score,
                         (void *) criteria,
                         (void *) score);
  }
}
