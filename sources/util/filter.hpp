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
#include "attribute.hpp"

class Module;

class Filter : public virtual Object
{
  public:
    struct Layout
    {
      AttributeDesc::Look  _look;
      AttributeDesc       *_desc;
    };

    Filter (GSList *attr_list,
            Module *owner = NULL);

    void ShowAttribute (const gchar *name);

    void UpdateAttrList ();

    void SelectAttributes ();

    GSList *GetAttrList ();

    GSList *GetLayoutList ();

    guint GetAttributeId (const gchar *name);

    void SetOwner (Module *owner);

    void UnPlug ();

  private:
    typedef enum
    {
      ATTR_VISIBILITY = 0,
      ATTR_USER_NAME,
      ATTR_XML_NAME,
      ATTR_LOOK_IMAGE,
      ATTR_LOOK_VALUE,

      NUM_ATTR_COLS
    } StoreAttrColumn;

    typedef enum
    {
      LOOK_IMAGE = 0,
      LOOK_VALUE,

      NUM_LOOK_COLS
    } StoreLookColumn;

    Module       *_owner;
    GSList       *_attr_list;
    GSList       *_selected_list;
    GtkListStore *_attr_filter_store;
    GtkWidget    *_dialog;
    GtkListStore *_look_store;

    virtual ~Filter ();

    static void OnVisibilityToggled (GtkCellRendererToggle *cell,
                                     gchar                 *path_string,
                                     Filter                *filter);

    static void OnLookChanged (GtkCellRendererCombo *combo,
                               gchar                *path_string,
                               GtkTreeIter          *new_iter,
                               Filter               *filter);

    static void OnAttrDeleted (GtkTreeModel *tree_model,
                               GtkTreePath  *path,
                               Filter       *filter);

};

#endif
