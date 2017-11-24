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

#include <libxml/xmlwriter.h>

#include "util/attribute.hpp"
#include "util/attribute_desc.hpp"
#include "util/player.hpp"
#include "pool_proxy.hpp"
#include "fencer_proxy.hpp"

#include "criteria.hpp"

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
  Criteria::Criteria (GQuark  quark,
                      guint   pool_count,
                      guint   depth,
                      Object *owner)
    : Object ("Criteria")
  {
    _owner = owner;
    _quark = quark;
    _image = g_quark_to_string (quark);
    _depth = depth;

    _pool_count   = pool_count;
    _fencer_count = 0;
    _fencer_list  = NULL;

    _score_table = g_new0 (guint, pool_count);
    _pool_table  = g_new0 (PoolProxy *, pool_count);
    _errors      = NULL;
  }

  // --------------------------------------------------------------------------------
  Criteria::~Criteria ()
  {
    g_slist_free (_fencer_list);
    g_free       (_score_table);
    g_free       (_pool_table);
    g_list_free  (_errors);
  }

  // --------------------------------------------------------------------------------
  void Criteria::Use (FencerProxy *fencer)
  {
    _fencer_count++;

    _fencer_list = g_slist_append (_fencer_list,
                                   fencer);

    UpdateProfile ();

    fencer->SetCriteria (_depth,
                         this);
  }

  // --------------------------------------------------------------------------------
  GQuark Criteria::GetQuark ()
  {
    return _quark;
  }

  // --------------------------------------------------------------------------------
  gboolean Criteria::HasQuark (GQuark quark)
  {
    return _quark == quark;
  }

  // --------------------------------------------------------------------------------
  void Criteria::UpdateProfile ()
  {
    if (_fencer_count % _pool_count)
    {
      _big_profile._pool_count   = _fencer_count % _pool_count;
      _small_profile._pool_count = _pool_count - _big_profile._pool_count;

      _small_profile._score = _fencer_count / _pool_count;
      _big_profile._score   = _small_profile._score + 1;
    }
    else
    {
      _big_profile._pool_count = _pool_count;
      _big_profile._score      = _fencer_count / _pool_count;

      _small_profile._pool_count = 0;
      _small_profile._score      = 0;
    }
  }

  // --------------------------------------------------------------------------------
  void Criteria::ChangeScore (gint       delta,
                              PoolProxy *for_pool)
  {
    _pool_table[for_pool->_id-1]  = for_pool;
    _score_table[for_pool->_id-1] += delta;
  }

  // --------------------------------------------------------------------------------
  gboolean Criteria::FreePlaceIn (PoolProxy *pool)
  {
    guint score = _score_table[pool->_id-1];

    if (_small_profile._score)
    {
      if (score < _small_profile._score)
      {
        return TRUE;
      }
    }
    else if (score < _big_profile._score)
    {
      return TRUE;
    }

    if (score == (_big_profile._score - 1))
    {
      guint over_populated = 0;

      for (guint i = 0; i < _pool_count; i ++)
      {
        if (_score_table[i] >= _big_profile._score)
        {
          over_populated++;
          if (over_populated >= _big_profile._pool_count)
          {
            return FALSE;
          }
        }
      }
      return TRUE;
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  GList *Criteria::GetErrors (GList *errors)
  {
    GList *own_errors          = NULL;
    GList *over_populated_list = NULL;

    for (guint i = 0; i < _pool_count; i++)
    {
      PoolProxy *pool = _pool_table[i];

      if (_score_table[i] > _big_profile._score)
      {
        GList *pool_errors = pool->GetFencers (_big_profile._score,
                                               _depth,
                                               _quark);

        own_errors = g_list_concat (own_errors,
                                    pool_errors);
      }
      if (_small_profile._score && (_score_table[i] > _small_profile._score))
      {
        over_populated_list = g_list_prepend (over_populated_list,
                                              pool);
      }
    }

    //printf (GREEN "  big  ==> %dx%d\n", _big_profile._pool_count, _big_profile._score);
    //printf (RED   "  over ==> %d\n" ESC, g_list_length (over_populated_list));

    {
      guint  remaining_errors = 0;
      GList *current          = over_populated_list;

      if (g_list_length (over_populated_list) > _big_profile._pool_count)
      {
        remaining_errors = g_list_length (over_populated_list) - _big_profile._pool_count;

        if (remaining_errors > g_list_length (own_errors))
        {
          remaining_errors -= g_list_length (own_errors);
        }
        else if (own_errors)
        {
          remaining_errors = 0;
        }
      }

      for (guint i = 0; i < remaining_errors; i++)
      {
        PoolProxy   *pool   = (PoolProxy *) current->data;
        FencerProxy *fencer = pool->GetFencer (_big_profile._score,
                                               _depth,
                                               _quark);
        if (fencer)
        {
          PRINT (YELLOW "  %02d  %s", pool->_id, fencer->_player->GetName ());
          own_errors = g_list_prepend (own_errors,
                                       fencer);
        }
        current = g_list_next (current);
      }
    }

    {
      GList *current = own_errors;

      while (current)
      {
        if (g_list_find (errors,
                         current->data) == FALSE)
        {
          FencerProxy *fencer = (FencerProxy *) current->data;

          errors = g_list_prepend (errors,
                                   fencer);
        }
        current = g_list_next (current);
      }
      g_list_free (own_errors);
    }

    return errors;
  }

  // --------------------------------------------------------------------------------
  void Criteria::Dump (GQuark         quark,
                       Criteria *criteria_value)
  {
    PRINT ("%d x %30s >> " GREEN "%dx%d" ESC " + " RED "%dx%d" ESC,
           criteria_value->_fencer_count,
           g_quark_to_string (quark),
           criteria_value->_small_profile._pool_count,
           criteria_value->_small_profile._score,
           criteria_value->_big_profile._pool_count,
           criteria_value->_big_profile._score);
  }
}
