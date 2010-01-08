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

#ifndef filter_hpp
#define filter_hpp

#include <gtk/gtk.h>

#include "object.hpp"

class Module_c;

class Filter : public virtual Object_c
{
  public:
    Filter (Module_c *owner);

    void ShowAttribute (gchar *name);

    void UpdateAttrList ();

    void SelectAttributes ();

    GSList *GetSelectedAttr ();

  private:
    typedef enum
    {
      ATTR_VISIBILITY = 0,
      ATTR_NAME,
      NUM_COLS
    } StoreColumn;

    Module_c     *_owner;
    GSList       *_selected_attr;
    GtkTreeModel *_attr_filter_store;
    GtkWidget    *_filter_window;

    virtual ~Filter ();

    static void on_cell_toggled (GtkCellRendererToggle *cell,
                                 gchar                 *path_string,
                                 gpointer               user_data);

    static void OnAttrDeleted (GtkTreeModel *tree_model,
                               GtkTreePath  *path,
                               gpointer      user_data);

};

#endif
