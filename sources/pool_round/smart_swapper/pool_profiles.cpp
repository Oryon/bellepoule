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

#include "pool_profiles.hpp"

namespace SmartSwapper
{
  // --------------------------------------------------------------------------------
  void PoolProfiles::Configure (guint nb_fencer,
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

    _sizes[SMALL] = _min_size;
    _sizes[BIG]   = _max_size;
  }

  // --------------------------------------------------------------------------------
  guint PoolProfiles::GetSize (guint type)
  {
    return _sizes[type];
  }

  // --------------------------------------------------------------------------------
  gboolean PoolProfiles::Exists (guint type)
  {
    return ((type < PROFILE_TYPE_LEN) && (_sizes[type] > 0));
  }

  // --------------------------------------------------------------------------------
  void PoolProfiles::ChangeFencerCount (guint old_count,
                                        guint new_count)
  {
    if (old_count < new_count)
    {
      if (new_count == _max_size)
      {
        _nb_max_reached++;
      }
    }
    else if (old_count > new_count)
    {
      if (old_count == _max_size)
      {
        _nb_max_reached--;
      }
    }

    if (_nb_max_reached >= _nb_max)
    {
      _sizes[BIG] = 0;
    }
    else
    {
      _sizes[BIG] = _max_size;
    }
  }
}
