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

#pragma once

#include <gtk/gtk.h>

#include "util/object.hpp"
#include "util/player.hpp"
#include "team.hpp"

namespace People
{
  class PlayersStore : public Object
  {
    public:
      PlayersStore (gint   n_columns,
                    GType *types);

      void Append (Player *player);

      void Remove (Player *player);

      void Update (Player *player);

      gboolean SelectFlatMode (GtkTreeView *view);

      void SetSearchColumn (gint column);

      gboolean SelectTreeMode (GtkTreeView *view);

      void Wipe ();

      void SetSortFunction (gint                    sort_column_id,
                            GtkTreeIterCompareFunc  sort_func,
                            Object                 *user_data);

      void SetVisibileFunc (GtkTreeModelFilterVisibleFunc func,
                            gpointer                      data);

      void Refilter ();

      GtkTreeStore *GetTreeStore (GtkTreeView *view);

      GtkTreePath *GetStorePathFromViewPath (GtkTreeView *view,
                                             GtkTreePath *view_path);

      GtkTreeRowReference *GetTreeRowRef (GtkTreeView *view,
                                          Player      *player);

    private:
      class StoreObject : public Object
      {
        public:
          GtkTreeModel *_gtk_tree_filter;
          GtkTreeModel *_gtk_tree_sort;
          GtkTreeStore *_gtk_tree_store;

          StoreObject (gint   n_columns,
                       GType *types);

          void Wipe ();

        private:
          ~StoreObject ();
      };

      ~PlayersStore ();

      void Append (StoreObject *store,
                   Player      *player,
                   Team        *team);

      void Remove (StoreObject *store,
                   Player      *player);

      GtkTreeRowReference *GetPlayerRowRef (GtkTreeIter  *iter,
                                            GtkTreeStore *store);

    private:
      StoreObject *_flat_store;
      StoreObject *_tree_store;
      gint         _search_column;
  };
}
