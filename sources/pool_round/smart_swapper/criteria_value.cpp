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

#include "criteria_value.hpp"

namespace SmartSwapper
{
  // --------------------------------------------------------------------------------
  CriteriaValue::CriteriaValue (Player::AttributeId *criteria_id)
    : Object ("CriteriaValue")
  {
    _fencer_count = 0;
    _fencer_list  = NULL;

    _criteria_id = criteria_id;
    _criteria_id->Retain ();
  }

  // --------------------------------------------------------------------------------
  CriteriaValue::~CriteriaValue ()
  {
    g_slist_free (_fencer_list);
    _criteria_id->Release ();
  }

  // --------------------------------------------------------------------------------
  void CriteriaValue::Use (Player *fencer)
  {
    _fencer_count++;

    _fencer_list = g_slist_insert_sorted_with_data (_fencer_list,
                                                    fencer,
                                                    (GCompareDataFunc) Player::Compare,
                                                    _criteria_id);
  }

  // --------------------------------------------------------------------------------
  gboolean CriteriaValue::HasFloatingProfile ()
  {
    return _floating_count > 0;
  }

  // --------------------------------------------------------------------------------
  guint CriteriaValue::GetErrorLine (Fencer *fencer)
  {
    if (_floating_count == 0)
    {
      return _max_count_per_pool+1;
    }
    else
    {
      if (CanFloat (fencer))
      {
        return _max_count_per_pool+1;
      }
      else
      {
        return _max_count_per_pool;
      }
    }
  }

  // --------------------------------------------------------------------------------
  gboolean CriteriaValue::CanFloat (Fencer *fencer)
  {
    if (_floating_count > 0)
    {
      guint rank = g_slist_index (_fencer_list, fencer->_player) + 1;

      if (rank > ((_max_count_per_pool-1) * _pool_count))
      {
        return TRUE;
      }
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void CriteriaValue::Profile (GQuark         quark,
                               CriteriaValue *criteria_value,
                               guint          pool_count)
  {
    criteria_value->_pool_count         = pool_count;
    criteria_value->_max_count_per_pool = criteria_value->_fencer_count / pool_count;

    criteria_value->_floating_count = criteria_value->_fencer_count % pool_count;
    if (criteria_value->_floating_count)
    {
      criteria_value->_max_count_per_pool++;
    }

    printf ("%d x %20s >> %d (max) %d (floating)\n", criteria_value->_fencer_count, g_quark_to_string (quark),
            criteria_value->_max_count_per_pool, criteria_value->_floating_count);
  }
}
