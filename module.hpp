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

#ifndef module_hpp
#define module_hpp

#include <gtk/gtk.h>

#include "object.hpp"
#include "glade.hpp"

class Module_c : public virtual Object_c
{
  public:
    virtual ~Module_c ();

    void Plug (Module_c   *module,
               GtkWidget  *in,
               GtkToolbar *toolbar = NULL);
    void UnPlug ();

    void SelectAttributes ();

    void CloneFilterList (Module_c *from);

    GtkWidget *GetConfigWidget ();

  protected:
    Glade_c *_glade;
    GSList  *_attr_list;

    Module_c (gchar *glade_file,
              gchar *root = NULL);

    virtual void OnPlugged   ();
    virtual void OnUnPlugged ();

    GtkWidget *GetRootWidget ();
    GtkToolbar *GetToolbar ();

    void AddSensitiveWidget (GtkWidget *w);
    void EnableSensitiveWidgets ();
    void DisableSensitiveWidgets ();

    void ShowAttribute (gchar *name);

    Module_c ();

  private:
    typedef enum
    {
      ATTR_VISIBILITY = 0,
      ATTR_NAME,
      NUM_COLS
    } StoreColumn;

    GtkWidget    *_root;
    GtkToolbar   *_toolbar;
    GSList       *_sensitive_widgets;
    GSList       *_plugged_list;
    Module_c     *_owner;
    gchar        *_name;
    GtkTreeModel *_attr_filter_store;
    GtkWidget    *_filter_window;
    GtkWidget    *_config_widget;

    void UpdateAttrList ();

    static void OnAttrDeleted (GtkTreeModel *tree_model,
                               GtkTreePath  *path,
                               gpointer      user_data);

    static void on_cell_toggled (GtkCellRendererToggle *cell,
                                 gchar                 *path_string,
                                 gpointer               user_data);

    virtual void OnAttrListUpdated () {};
};

#endif
