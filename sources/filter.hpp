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

class Module;

class Filter : public virtual Object
{
  public:
    Filter (GSList *attr_list,
            Module *owner = NULL);

    void SetAttributeList (GSList *list);

    void ShowAttribute (const gchar *name);

    void UpdateAttrList ();

    void SelectAttributes ();

    GSList *GetAttrList ();

    GSList *GetSelectedAttrList ();

    guint GetAttributeId (const gchar *name);

    void SetOwner (Module *owner);

    void UnPlug ();

  private:
    typedef enum
    {
      ATTR_VISIBILITY = 0,
      ATTR_USER_NAME,
      ATTR_XML_NAME,
      NUM_COLS
    } StoreColumn;

    Module       *_owner;
    GSList       *_attr_list;
    GSList       *_selected_attr;
    GtkListStore *_attr_filter_store;
    GtkWidget    *_dialog;

    virtual ~Filter ();

    static void on_cell_toggled (GtkCellRendererToggle *cell,
                                 gchar                 *path_string,
                                 gpointer               user_data);

    static void OnAttrDeleted (GtkTreeModel *tree_model,
                               GtkTreePath  *path,
                               gpointer      user_data);

};

#endif
