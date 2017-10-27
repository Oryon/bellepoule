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
#include <libxml/xmlwriter.h>

#include "object.hpp"
#include "attribute.hpp"

class Module;

class Filter : public Object
{
  public:
    class Layout : public Object
    {
      public:
        Layout ()
          : Object ("Filter::Layout")
        {
        };

        AttributeDesc::Look  _look;
        AttributeDesc       *_desc;
    };

    Filter (const gchar *name,
            GSList      *attr_list);

    void ShowAttribute (const gchar *name);

    void UpdateAttrList (gboolean save_it = TRUE);

    void SelectAttributes ();

    void RestoreLast ();

    GSList *GetAttrList ();

    GList *GetLayoutList ();

    guint GetAttributeId (const gchar *name);

    void AddOwner (Module *owner);

    void UnPlug ();

    void Save (xmlTextWriter *xml_writer,
               const gchar   *as = "");

    void Load (xmlNode *xml_node,
               const gchar   *as = "");

    static void PreventPersistence ();

  private:
    typedef enum
    {
      ATTR_VISIBILITY_bool = 0,
      ATTR_USER_NAME_str,
      ATTR_XML_NAME_ptr,
      ATTR_LOOK_IMAGE_str,
      ATTR_LOOK_VALUE_uint,

      NUM_ATTR_COLS
    } StoreAttrColumn;

    typedef enum
    {
      LOOK_IMAGE_str = 0,
      LOOK_VALUE_uint,

      NUM_LOOK_COLS
    } StoreLookColumn;

    static gboolean _no_persistence;

    GList        *_owners;
    gchar        *_name;
    GSList       *_attr_list;
    GList        *_selected_list;
    GtkListStore *_attr_filter_store;
    GtkWidget    *_dialog;
    GtkListStore *_look_store;

    virtual ~Filter ();

    void ApplyList (gchar **list);

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
