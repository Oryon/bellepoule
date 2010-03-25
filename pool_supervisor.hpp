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

#include "data.hpp"
#include "module.hpp"
#include "pool.hpp"

class PoolAllocator;

class PoolSupervisor : public virtual Stage, public Module
{
  public:
    static void Init ();

    PoolSupervisor (StageClass *stage_class);

    void OnPrintPoolToolbuttonClicked ();

    void OnPoolSelected (gint index);

    void OnFilterClicked ();

    void OnStuffClicked ();

  private:
    void Display ();
    void Garnish ();
    void LoadConfiguration (xmlNode *xml_node);
    void Load (xmlNode *xml_node);
    void Load (xmlNode *xml_node,
               guint    current_pool_index);
    void Save (xmlTextWriter *xml_writer);
    void OnLocked (Reason reason);
    void OnUnLocked ();
    void Wipe ();
    void RetrievePools ();
    void Manage (Pool *pool);

  private:
    void OnPoolSelected (Pool *pool);

    static Stage *CreateInstance (StageClass *stage_class);

    void FillInConfig ();
    void ApplyConfig ();
    Stage *GetInputProvider ();
    gboolean IsOver ();

  private:
    static const gchar *_class_name;
    static const gchar *_xml_class_name;

    GtkListStore  *_pool_liststore;
    PoolAllocator *_pool_allocator;
    Pool          *_displayed_pool;
    Data          *_max_score;

    ~PoolSupervisor ();

    void OnAttrListUpdated ();

    void OnPlugged ();

    void OnUnPlugged ();

    GSList *GetCurrentClassification ();

    void SetInputProvider (Stage *input_provider);

    void SaveConfiguration (xmlTextWriter *xml_writer);

    static void OnPoolStatusUpdated (Pool           *pool,
                                     PoolSupervisor *ps);

    static gint ComparePlayer (Player         *A,
                               Player         *B,
                               PoolSupervisor *pool_supervisor);
};

#endif

