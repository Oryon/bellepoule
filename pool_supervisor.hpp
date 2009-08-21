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

#ifndef pool_supervisor_hpp
#define pool_supervisor_hpp

#include <gtk/gtk.h>

#include "module.hpp"
#include "pool.hpp"

class PoolsList_c;

class PoolSupervisor_c : public virtual Stage_c, public Module_c
{
  public:
    PoolSupervisor_c (gchar *name);

    void Manage (Pool_c *pool);

    void OnPrintPoolToolbuttonClicked ();

  private:
    void Load (xmlDoc *doc);
    void Save (xmlTextWriter *xml_writer);
    void Enter ();
    void OnLocked ();
    void OnUnLocked ();
    void Wipe ();

  private:
    static gboolean on_pool_selected (GtkWidget      *widget,
                                      GdkEventButton *event,
                                      gpointer        user_data);
    void OnPoolSelected (Pool_c *pool);

  private:
    GtkWidget   *_menu_pool;
    PoolsList_c *_pool_allocator;
    Pool_c      *_displayed_pool;

    ~PoolSupervisor_c ();
};

#endif

