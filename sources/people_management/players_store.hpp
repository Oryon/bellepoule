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

#ifndef players_store_hpp
#define players_store_hpp

#include <gtk/gtk.h>

#include "util/object.hpp"

namespace People
{
  class PlayersStore : public Object
  {
    public:
      PlayersStore (gint   n_columns,
                    GType *types);

      void Append (Object *item,
                   Object *parent);

      void Remove (Object *item);

      gboolean SelectFlatMode (GtkTreeView *view);

      gboolean SelectTreeMode (GtkTreeView *view);

      void Wipe ();

      void SetSortFunction (gint                    sort_column_id,
                            GtkTreeIterCompareFunc  sort_func,
                            Object                 *user_data);

      GtkTreeRowReference *GetTreeRowRef (GtkTreeModel *store,
                                          Object       *item);

    private:
      struct StoreObject : public Object
      {
        GtkTreeStore *_gtk_tree_store;
      };

      ~PlayersStore ();

      void Append (StoreObject *store,
                   Object      *item,
                   Object      *parent);

      void Remove (StoreObject *store,
                   Object      *item);

      GtkTreeRowReference *GetPlayerRowRef (GtkTreeIter  *iter,
                                            GtkTreeStore *store);

    private:
      StoreObject _flat_store;
      StoreObject _tree_store;
  };
}

#endif
