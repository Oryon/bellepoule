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

#ifndef pool_data_hpp
#define pool_data_hpp

#include "util/object.hpp"
#include "pool_round/pool.hpp"

#include "pool_profiles.hpp"

namespace SmartSwapper
{
  class Fencer;

  class PoolData
  {
    public:
      Pool::Pool    *_pool;
      guint          _criteria_count;
      GHashTable   **_criteria_scores;
      GHashTable   **_original_criteria_scores;
      GList         *_fencer_list;
      guint          _size;
      guint          _id;
      PoolProfiles  *_pool_profiles;

      PoolData (Pool::Pool   *pool,
                guint         id,
                PoolProfiles *profiles,
                guint         criteria_count);

      ~PoolData ();

      void AddFencer (Fencer *fencer);

      void RemoveFencer (Fencer *fencer);

    private:
      void ChangeCriteriaScore (GQuark      criteria,
                                GHashTable *criteria_score,
                                gint        delta_score);
  };
}

#endif
