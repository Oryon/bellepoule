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

#ifndef smart_sizes_hpp
#define smart_sizes_hpp

#include "util/object.hpp"

namespace SmartSwapper
{
  class PoolSizes
  {
    public:
      typedef enum
      {
        MIN_SIZE,
        MAX_SIZE,
        END_MARK,

        SIZE_TYPE_LEN
      } SizeType;

      void Configure (guint nb_fencer,
                      guint nb_pool);

      void NewSize (guint old_size,
                    guint new_size);

      guint _available_sizes[SIZE_TYPE_LEN];
      guint _min_size;
      guint _max_size;

    private:
      guint _nb_max;
      guint _nb_max_reached;
  };
}

#endif
