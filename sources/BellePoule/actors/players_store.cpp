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

#include "util/attribute_desc.hpp"

#include "fencer.hpp"
#include "team.hpp"
#include "players_store.hpp"

namespace People
{
  // --------------------------------------------------------------------------------
  PlayersStore::StoreObject::StoreObject (gint   n_columns,
                                          GType *types)
    : Object ("PlayersStore::StoreObject")
  {
    _gtk_tree_store = gtk_tree_store_newv (n_columns,
                                           types);
    _gtk_tree_filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (_gtk_tree_store),
                                                  NULL);
    _gtk_tree_sort = gtk_tree_model_sort_new_with_model (_gtk_tree_filter);
  }

  // --------------------------------------------------------------------------------
  PlayersStore::StoreObject::~StoreObject ()
  {
    g_object_unref (G_OBJECT (_gtk_tree_filter));
    g_object_unref (G_OBJECT (_gtk_tree_store));
  }

  // --------------------------------------------------------------------------------
  void PlayersStore::StoreObject::Wipe ()
  {
    gtk_tree_store_clear (_gtk_tree_store);
  }
}

namespace People
{
  // --------------------------------------------------------------------------------
  PlayersStore::PlayersStore (gint   n_columns,
                              GType *types)
    : Object ("PlayersStore")
  {
    _flat_store = new StoreObject (n_columns,
                                   types);
    _tree_store = new StoreObject (n_columns,
                                   types);
  }

  // --------------------------------------------------------------------------------
  PlayersStore::~PlayersStore ()
  {
    _flat_store->Release ();
    _tree_store->Release ();
  }

  // --------------------------------------------------------------------------------
  void PlayersStore::SetSearchColumn (gint column)
  {
    _search_column = column * AttributeDesc::NB_LOOK;
  }

  // --------------------------------------------------------------------------------
  gboolean PlayersStore::SelectFlatMode (GtkTreeView *view)
  {
    if (GTK_TREE_MODEL (_flat_store->_gtk_tree_sort) != gtk_tree_view_get_model (view))
    {
      gtk_tree_view_set_model (view,
                               GTK_TREE_MODEL (_flat_store->_gtk_tree_sort));
      gtk_tree_view_set_search_column (view,
                                       _search_column);
      return TRUE;
    }
    return FALSE;
  }

  // --------------------------------------------------------------------------------
  gboolean PlayersStore::SelectTreeMode (GtkTreeView *view)
  {
    if (GTK_TREE_MODEL (_tree_store->_gtk_tree_sort) != gtk_tree_view_get_model (view))
    {
      gtk_tree_view_set_model (view,
                               GTK_TREE_MODEL (_tree_store->_gtk_tree_sort));
      gtk_tree_view_set_search_column (view,
                                       _search_column);
      return TRUE;
    }
    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void PlayersStore::Update (Player *player)
  {
    Remove (player);
    Append (player);
  }

  // --------------------------------------------------------------------------------
  void PlayersStore::Append (Player *player)
  {
    Team *team = NULL;

    if (player->Is ("Team") == FALSE)
    {
      Append (_flat_store,
              player,
              NULL);

      if (player->Is ("Fencer"))
      {
        Fencer *fencer = (Fencer *) player;

        team = fencer->GetTeam ();
      }
    }

    Append (_tree_store,
            player,
            team);
  }

  // --------------------------------------------------------------------------------
  void PlayersStore::Append (StoreObject *store,
                             Player      *player,
                             Team        *team)
  {
    GtkTreeIter team_iter;
    gboolean    has_team = FALSE;

    {
      if (team)
      {
        GtkTreeRowReference *row_ref = (GtkTreeRowReference *) team->GetPtrData (store, "tree_row_ref");

        if (row_ref)
        {
          GtkTreePath *path = gtk_tree_row_reference_get_path (row_ref);

          if (path)
          {
            gtk_tree_model_get_iter (GTK_TREE_MODEL (store->_gtk_tree_store),
                                     &team_iter,
                                     path);
            has_team = TRUE;
          }
          gtk_tree_path_free (path);
        }
      }
    }

    {
      GtkTreeIter iter;

      if (has_team)
      {
        gtk_tree_store_append (store->_gtk_tree_store,
                               &iter,
                               &team_iter);
      }
      else
      {
        gtk_tree_store_append (store->_gtk_tree_store,
                               &iter,
                               NULL);
      }

      player->SetData (store, "tree_row_ref",
                       GetPlayerRowRef (&iter, store->_gtk_tree_store),
                       (GDestroyNotify) gtk_tree_row_reference_free);
    }
  }

  // --------------------------------------------------------------------------------
  void PlayersStore::Remove (Player *player)
  {
    Remove (_flat_store,
            player);
    Remove (_tree_store,
            player);
  }

  // --------------------------------------------------------------------------------
  void PlayersStore::Remove (StoreObject  *store,
                             Player       *player)
  {
    GtkTreeRowReference *ref = (GtkTreeRowReference *) player->GetPtrData (store, "tree_row_ref");

    if (ref)
    {
      GtkTreePath *path = gtk_tree_row_reference_get_path (ref);

      if (path)
      {
        GtkTreeIter iter;

        gtk_tree_model_get_iter (GTK_TREE_MODEL (store->_gtk_tree_store),
                                 &iter,
                                 path);

        gtk_tree_store_remove (store->_gtk_tree_store,
                               &iter);
      }
      gtk_tree_path_free (path);

      player->RemoveData (store,
                          "tree_row_ref");
    }
  }

  // --------------------------------------------------------------------------------
  GtkTreePath *PlayersStore::GetStorePathFromViewPath (GtkTreeView *view,
                                                       GtkTreePath *view_path)
  {
    GtkTreeModel *model_sort = gtk_tree_view_get_model (view);
    GtkTreePath  *sort_path;
    GtkTreePath  *store_path;

    if (_flat_store->_gtk_tree_sort == model_sort)
    {
      sort_path = gtk_tree_model_sort_convert_path_to_child_path (GTK_TREE_MODEL_SORT (_flat_store->_gtk_tree_sort),
                                                                  view_path);
      store_path = gtk_tree_model_filter_convert_path_to_child_path (GTK_TREE_MODEL_FILTER (_flat_store->_gtk_tree_filter),
                                                                                            sort_path);
    }
    else
    {
      sort_path = gtk_tree_model_sort_convert_path_to_child_path (GTK_TREE_MODEL_SORT (_tree_store->_gtk_tree_sort),
                                                                  view_path);
      store_path = gtk_tree_model_filter_convert_path_to_child_path (GTK_TREE_MODEL_FILTER (_tree_store->_gtk_tree_filter),
                                                                                            sort_path);
    }
    gtk_tree_path_free (sort_path);

    return store_path;
  }

  // --------------------------------------------------------------------------------
  GtkTreeStore *PlayersStore::GetTreeStore (GtkTreeView *view)
  {
    GtkTreeModel *model = gtk_tree_view_get_model (view);

    if (_flat_store->_gtk_tree_sort == model)
    {
      return _flat_store->_gtk_tree_store;
    }
    else
    {
      return _tree_store->_gtk_tree_store;
    }
  }

  // --------------------------------------------------------------------------------
  GtkTreeRowReference *PlayersStore::GetTreeRowRef (GtkTreeView *view,
                                                    Player      *player)
  {
    GtkTreeModel *model = gtk_tree_view_get_model (view);

    if (_flat_store->_gtk_tree_sort == model)
    {
      return (GtkTreeRowReference *) player->GetPtrData (_flat_store,
                                                         "tree_row_ref");
    }
    else
    {
      return (GtkTreeRowReference *) player->GetPtrData (_tree_store,
                                                         "tree_row_ref");
    }
  }

  // --------------------------------------------------------------------------------
  GtkTreeRowReference *PlayersStore::GetPlayerRowRef (GtkTreeIter  *iter,
                                                      GtkTreeStore *store)
  {
    GtkTreeRowReference *ref;
    GtkTreePath         *path = gtk_tree_model_get_path (GTK_TREE_MODEL (store),
                                                         iter);

    ref = gtk_tree_row_reference_new (GTK_TREE_MODEL (store),
                                      path);

    gtk_tree_path_free (path);

    return ref;
  }

  // --------------------------------------------------------------------------------
  void PlayersStore::Wipe ()
  {
    _flat_store->Wipe ();
    _tree_store->Wipe ();
  }

  // --------------------------------------------------------------------------------
  void PlayersStore::SetSortFunction (gint                    sort_column_id,
                                      GtkTreeIterCompareFunc  sort_func,
                                      Object                 *user_data)
  {
    user_data->Retain ();
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (_flat_store->_gtk_tree_sort),
                                     sort_column_id,
                                     sort_func,
                                     user_data,
                                     (GDestroyNotify) Object::TryToRelease);

    user_data->Retain ();
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (_tree_store->_gtk_tree_sort),
                                     sort_column_id,
                                     sort_func,
                                     user_data,
                                     (GDestroyNotify) Object::TryToRelease);
  }

  // --------------------------------------------------------------------------------
  void PlayersStore::Refilter ()
  {
    gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (_flat_store->_gtk_tree_filter));
    gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (_tree_store->_gtk_tree_filter));
  }

  // --------------------------------------------------------------------------------
  void PlayersStore::SetVisibileFunc (GtkTreeModelFilterVisibleFunc func,
                                      gpointer                      data)
  {
    gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (_flat_store->_gtk_tree_filter),
                                            func,
                                            data,
                                            NULL);
    gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (_tree_store->_gtk_tree_filter),
                                            func,
                                            data,
                                            NULL);
  }
}
