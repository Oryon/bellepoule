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

#include "tree_model_index.hpp"

// --------------------------------------------------------------------------------
TreeModelIndex::TreeModelIndex (GtkTreeModel *model,
                                GHashFunc     key_type)
: Object ("TreeModelIndex")
{
  _model    = model;
  _key_type = key_type;

  if (key_type == g_str_hash)
  {
    _hash_table = g_hash_table_new_full (g_str_hash,
                                         g_str_equal,
                                         g_free,
                                         (GDestroyNotify) gtk_tree_row_reference_free);
  }
  else
  {
    _hash_table = g_hash_table_new_full (NULL,
                                         NULL,
                                         NULL,
                                         (GDestroyNotify) gtk_tree_row_reference_free);
  }
}

// --------------------------------------------------------------------------------
TreeModelIndex::~TreeModelIndex ()
{
  g_hash_table_destroy (_hash_table);
}

// --------------------------------------------------------------------------------
void TreeModelIndex::SetIterToKey (GtkTreeIter *iter,
                                   const void  *key)
{
  GtkTreePath *path = gtk_tree_model_get_path (_model,
                                               iter);

  if (path)
  {
    GtkTreeRowReference *row_ref = gtk_tree_row_reference_new (_model,
                                                               path);

    if (_key_type == g_str_hash)
    {
      g_hash_table_insert (_hash_table,
                           g_strdup ((gchar *) key),
                           row_ref);
    }
    else
    {
      g_hash_table_insert (_hash_table,
                           (gpointer) key,
                           row_ref);
    }

    gtk_tree_path_free (path);
  }
}

// --------------------------------------------------------------------------------
gboolean TreeModelIndex::GetIterFromKey (const void  *key,
                                         GtkTreeIter *iter)
{
  GtkTreeRowReference *row_ref = (GtkTreeRowReference *) g_hash_table_lookup (_hash_table,
                                                                              key);

  if (row_ref)
  {
    GtkTreePath *path = gtk_tree_row_reference_get_path (row_ref);

    if (path)
    {
      gboolean result = gtk_tree_model_get_iter (_model,
                                                 iter,
                                                 path);
      gtk_tree_path_free (path);

      return result;
    }
  }

  return FALSE;
}

