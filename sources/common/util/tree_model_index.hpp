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

#ifndef tree_model_index_hpp
#define tree_model_index_hpp

#include <gtk/gtk.h>

#include "object.hpp"

class TreeModelIndex : public Object
{
  public:
    TreeModelIndex (GtkTreeModel *model,
                    GHashFunc     key_type);

    void SetIterToKey (GtkTreeIter *iter,
                       const void  *key);

    gboolean GetIterFromKey (const void  *key,
                             GtkTreeIter *iter);

  private:
    GtkTreeModel *_model;
    GHashTable   *_hash_table;
    GHashFunc     _key_type;

    virtual ~TreeModelIndex ();
};

#endif
