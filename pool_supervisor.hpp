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

class PoolAllocator_c;

class PoolSupervisor_c : public virtual Stage_c, public Module_c
{
  public:
    static void Init ();

    PoolSupervisor_c (StageClass *stage_class);

    void Manage (Pool_c *pool);

    void OnPrintPoolToolbuttonClicked ();

    void OnPoolSelected (gint index);

    void OnFilterClicked ();

  private:
    void Display ();
    void Garnish ();
    void Load (xmlNode *xml_node);
    void Save (xmlTextWriter *xml_writer);
    void OnLocked ();
    void OnUnLocked ();
    void Wipe ();
    void RetrievePools ();

  private:
    void OnPoolSelected (Pool_c *pool);

    static Stage_c *CreateInstance (StageClass *stage_class);

    void FillInConfig ();
    void ApplyConfig ();
    Stage_c *GetInputProvider ();
    gboolean IsOver ();

  private:
    static const gchar *_class_name;
    static const gchar *_xml_class_name;

    GtkListStore    *_pool_liststore;
    PoolAllocator_c *_pool_allocator;
    Pool_c          *_displayed_pool;
    guint            _max_score;

    ~PoolSupervisor_c ();

    void OnAttrListUpdated ();
    void OnPlugged ();

    static void OnPoolStatusUpdated (Pool_c           *pool,
                                     PoolSupervisor_c *ps);
};

#endif

