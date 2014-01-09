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
  CriteriaValue::CriteriaValue (Attribute *criteria_attr)
    : Object ("CriteriaValue")
  {
    _count       = 0;
    _fencer_list = NULL;

    _criteria_attr = criteria_attr;
    _criteria_attr->Retain ();
  }

  // --------------------------------------------------------------------------------
  CriteriaValue::~CriteriaValue ()
  {
    g_slist_free (_fencer_list);
    Object::TryToRelease (_criteria_attr);
  }

  // --------------------------------------------------------------------------------
  void CriteriaValue::Use (Player *fencer)
  {
    _count++;
  }

  // --------------------------------------------------------------------------------
  gboolean CriteriaValue::HasFloatingProfile ()
  {
    return _max_floating_count > 0;
  }

  // --------------------------------------------------------------------------------
  void CriteriaValue::Profile (GQuark         quark,
                               CriteriaValue *criteria_value,
                               guint          pool_count)
  {
    criteria_value->_max_criteria_count = criteria_value->_count / pool_count;

    criteria_value->_max_floating_count = criteria_value->_count % pool_count;
    if (criteria_value->_max_floating_count)
    {
      criteria_value->_max_criteria_count++;
    }

    printf ("%d x %20s >> %d (max) %d (floating)", criteria_value->_count, g_quark_to_string (quark),
            criteria_value->_max_criteria_count, criteria_value->_max_floating_count);
  }
}
