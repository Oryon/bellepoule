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
    _small_size    = nb_fencer / nb_pool;
    _big_size      = 0;
    _big_count     = 0;
    _max_big_count = nb_fencer % nb_pool;
    if (_max_big_count)
    {
      _big_size = _small_size+1;
    }

    _sizes[SMALL] = _small_size;
    _sizes[BIG]   = _big_size;
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
      if (new_count == _big_size)
      {
        _big_count++;
      }
    }
    else if (old_count > new_count)
    {
      if (old_count == _big_size)
      {
        _big_count--;
      }
    }

    if (_big_count >= _max_big_count)
    {
      _sizes[BIG] = 0;
    }
    else
    {
      _sizes[BIG] = _big_size;
    }
  }
}
