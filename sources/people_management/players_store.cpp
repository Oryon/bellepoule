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

#include "players_store.hpp"

namespace People
{
  // --------------------------------------------------------------------------------
  PlayersStore::PlayersStore (gint   n_columns,
                              GType *types)
    : Object ("PlayersStore")
  {
    _flat_store._gtk_tree_store = gtk_tree_store_newv (n_columns,
                                                       types);
    _tree_store._gtk_tree_store = gtk_tree_store_newv (n_columns,
                                                       types);
  }

  // --------------------------------------------------------------------------------
  PlayersStore::~PlayersStore ()
  {
    g_object_unref (_flat_store._gtk_tree_store);
    g_object_unref (_tree_store._gtk_tree_store);
  }

  // --------------------------------------------------------------------------------
  gboolean PlayersStore::SelectFlatMode (GtkTreeView *view)
  {
    if (GTK_TREE_MODEL (_flat_store._gtk_tree_store) != gtk_tree_view_get_model (view))
    {
      gtk_tree_view_set_model (view,
                               GTK_TREE_MODEL (_flat_store._gtk_tree_store));
      return TRUE;
    }
    return FALSE;
  }

  // --------------------------------------------------------------------------------
  gboolean PlayersStore::SelectTreeMode (GtkTreeView *view)
  {
    if (GTK_TREE_MODEL (_tree_store._gtk_tree_store) != gtk_tree_view_get_model (view))
    {
      gtk_tree_view_set_model (view,
                               GTK_TREE_MODEL (_tree_store._gtk_tree_store));
      return TRUE;
    }
    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void PlayersStore::Append (Object *item,
                             Object *parent)
  {
    Append (&_flat_store,
            item,
            NULL);
    Append (&_tree_store,
            item,
            parent);
  }

  // --------------------------------------------------------------------------------
  void PlayersStore::Append (StoreObject *store,
                             Object      *item,
                             Object      *parent)
  {
    GtkTreeIter parent_iter;
    gboolean    has_parent = FALSE;

    {
      if (parent)
      {
        GtkTreePath *path = gtk_tree_row_reference_get_path ((GtkTreeRowReference *) parent->GetPtrData (store,
                                                                                                         "tree_row_ref"));

        if (path)
        {
          gtk_tree_model_get_iter (GTK_TREE_MODEL (store->_gtk_tree_store),
                                   &parent_iter,
                                   path);
          has_parent = TRUE;
        }
        gtk_tree_path_free (path);
      }
    }

    {
      GtkTreeIter iter;

      if (has_parent)
      {
        gtk_tree_store_append (store->_gtk_tree_store,
                               &iter,
                               &parent_iter);
      }
      else
      {
        gtk_tree_store_append (store->_gtk_tree_store,
                               &iter,
                               NULL);
      }

      item->SetData (store, "tree_row_ref",
                     GetPlayerRowRef (&iter, store->_gtk_tree_store),
                     (GDestroyNotify) gtk_tree_row_reference_free);
    }
  }

  // --------------------------------------------------------------------------------
  void PlayersStore::Remove (Object *item)
  {
    Remove (&_flat_store,
            item);
    Remove (&_tree_store,
            item);
  }

  // --------------------------------------------------------------------------------
  void PlayersStore::Remove (StoreObject  *store,
                             Object       *item)
  {
    GtkTreePath *path = gtk_tree_row_reference_get_path ((GtkTreeRowReference *) item->GetPtrData (store,
                                                                                                   "tree_row_ref"));

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

    item->RemoveData (store,
                      "tree_row_ref");
  }

  // --------------------------------------------------------------------------------
  GtkTreeRowReference *PlayersStore::GetTreeRowRef (GtkTreeModel *store,
                                                    Object       *item)
  {
    if (_flat_store._gtk_tree_store == GTK_TREE_STORE (store))
    {
      return (GtkTreeRowReference *) item->GetPtrData (&_flat_store,
                                                       "tree_row_ref");
    }
    else
    {
      return (GtkTreeRowReference *) item->GetPtrData (&_tree_store,
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
    gtk_tree_store_clear (_flat_store._gtk_tree_store);
    gtk_tree_store_clear (_tree_store._gtk_tree_store);
  }

  // --------------------------------------------------------------------------------
  void PlayersStore::SetSortFunction (gint                    sort_column_id,
                                      GtkTreeIterCompareFunc  sort_func,
                                      Object                 *user_data)
  {
    user_data->Retain ();
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (_flat_store._gtk_tree_store),
                                     sort_column_id,
                                     sort_func,
                                     user_data,
                                     (GDestroyNotify) Object::TryToRelease);

    user_data->Retain ();
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (_tree_store._gtk_tree_store),
                                     sort_column_id,
                                     sort_func,
                                     user_data,
                                     (GDestroyNotify) Object::TryToRelease);
  }
}
