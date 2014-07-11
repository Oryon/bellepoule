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

#ifndef match_dispatcher_hpp
#define match_dispatcher_hpp

#include <glib.h>

#include "util/object.hpp"
#include "pair.hpp"

namespace Pool
{
  class MatchDispatcher : public Object
  {
    public:
      MatchDispatcher (guint pool_size);

      MatchDispatcher (guint        pool_size,
                       const gchar *name,
                       ...);

      void Dump ();

    private:
      GList *_pair_list;
      guint  _pool_size;
      gchar *_name;

      virtual ~MatchDispatcher ();

      void Init (const gchar *name,
                 guint        pool_size);

      GList *GetOpponentList (guint pool_size);

      void SpreadOpponents (GList *opponent_list);

      void CreatePairs (GList *opponent_list);

      void FixErrors ();

      gint GetFitness (Pair *pair);

      void RefreshFitness ();
  };
}

#endif
