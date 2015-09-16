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

#ifndef dispatcher_hpp
#define dispatcher_hpp

#include <glib.h>

#include "util/object.hpp"
#include "pair.hpp"

namespace Pool
{
  class Dispatcher : public Object
  {
    public:
      static const guint _MAX_POOL_SIZE = 17;

      Dispatcher (const gchar *name);

      Dispatcher (guint        pool_size,
                  const gchar *name,
                  ...);

      void SetAffinityCriteria (AttributeDesc *affinity_criteria,
                                GSList        *fencer_list);

      gboolean GetPlayerPair (guint     match_index,
                              guint    *a_id,
                              guint    *b_id,
                              gboolean *rest_error);

      void Dump ();

    private:
      GList *_pair_list;
      guint  _pool_size;
      gchar *_name;
      GList *_opponent_list;

      virtual ~Dispatcher ();

      void Reset ();

      void SpreadOpponents (GList *opponent_list);

      void CreatePairs (GList *opponent_list);

      void FixErrors ();

      void RefreshFitness ();

      guint GetPairCount ();

      void LockOpponents (GQuark teammate_quark,
                          guint  teammate_count);
  };
}

#endif
