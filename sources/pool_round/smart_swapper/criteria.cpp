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

#include "criteria.hpp"

namespace SmartSwapper
{
  // --------------------------------------------------------------------------------
  Criteria::Criteria()
    : Object ("Criteria")
  {
    _count = 0;
  }

  // --------------------------------------------------------------------------------
  Criteria::~Criteria()
  {
  }

  // --------------------------------------------------------------------------------
  void Criteria::Use ()
  {
    _count++;
  }

  // --------------------------------------------------------------------------------
  gboolean Criteria::HasFloatingProfile ()
  {
    return _max_floating_count > 0;
  }

  // --------------------------------------------------------------------------------
  void Criteria::Profile (GQuark    quark,
                          Criteria *criteria,
                          guint     pool_count)
  {
    criteria->_max_criteria_count = criteria->_count / pool_count;

    criteria->_max_floating_count = criteria->_count % pool_count;
    if (criteria->_max_floating_count)
    {
      criteria->_max_criteria_count++;
    }

    printf ("%d x %20s >> %d (max) %d (floating)", criteria->_count, g_quark_to_string (quark),
            criteria->_max_criteria_count, criteria->_max_floating_count);
  }
}
