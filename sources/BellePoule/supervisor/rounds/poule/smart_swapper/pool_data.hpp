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
#include "../pool.hpp"

#include "pool_profiles.hpp"

namespace SmartSwapper
{
  class Fencer;

  class PoolData : public Object
  {
    public:
      Pool::Pool  *_pool;
      GHashTable **_criteria_scores;
      GList       *_fencer_list;
      guint        _id;
      guint        _size;

    public:
      PoolData (Pool::Pool   *pool,
                guint         id,
                PoolProfiles *profiles,
                guint         criteria_count);

      void AddFencer (Fencer *fencer);

      void InsertFencer (Fencer *fencer);

      void RemoveFencer (Fencer *fencer);

      void SetError (guint  criteria_depth,
                     GQuark criteria_quark);

      gboolean HasErrorsFor (guint  criteria_depth,
                             GQuark criteria_quark);

      guint GetTeammateRank (Fencer *fencer,
                             guint   criteria_depth);

    private:
      guint          _criteria_count;
      GHashTable   **_criteria_errors;
      PoolProfiles  *_pool_profiles;

      ~PoolData ();

      void ChangeCriteriaScore (GQuark      criteria,
                                GHashTable *criteria_score,
                                gint        delta_score);
  };
}

#endif
