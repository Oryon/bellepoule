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

#include "pool_sizes.hpp"

namespace SmartSwapper
{
  // --------------------------------------------------------------------------------
  void PoolSizes::Configure (guint nb_fencer,
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

    _available_sizes[MIN_SIZE] = _min_size;
    _available_sizes[MAX_SIZE] = _max_size;
    _available_sizes[END_MARK] = 0;
  }

  // --------------------------------------------------------------------------------
  void PoolSizes::NewSize (guint old_size,
                           guint new_size)
  {
    if (old_size < new_size)
    {
      if (new_size == _max_size)
      {
        _nb_max_reached++;
      }
    }
    else if (old_size > new_size)
    {
      if (old_size == _max_size)
      {
        _nb_max_reached--;
      }
    }

    if (_nb_max_reached >= _nb_max)
    {
      _available_sizes[MAX_SIZE] = 0;
    }
    else
    {
      _available_sizes[MAX_SIZE] = _max_size;
    }
  }
}
