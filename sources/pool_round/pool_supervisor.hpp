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
    static void Declare ();

    PoolSupervisor (StageClass *stage_class);

    void OnPrintPoolToolbuttonClicked ();

    void OnPoolSelected (gint index);

    void OnFilterClicked ();

    void OnStuffClicked ();

    void OnToggleSingleClassification (gboolean single_selected);

    void OnScoreDeviceClicked ();

  private:
    void Display ();
    void Garnish ();
    void OnLoadingCompleted ();
    void OnLocked ();
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

    Object        *_single_owner;
    GtkListStore  *_pool_liststore;
    PoolAllocator *_pool_allocator;
    Pool          *_displayed_pool;
    GtkWidget     *_print_dialog;
    gboolean       _print_all_pool;

    virtual ~PoolSupervisor ();

    void OnAttrListUpdated ();

    void OnPlugged ();

    void OnUnPlugged ();

    void Load (xmlNode *xml_node);

    GSList *GetCurrentClassification ();

    GSList *EvaluateClassification (GSList           *list,
                                    Object           *rank_owner,
                                    GCompareDataFunc  CompareFunction);

    void SetInputProvider (Stage *input_provider);

    void OnBeginPrint (GtkPrintOperation *operation,
                       GtkPrintContext   *context);

    void OnDrawPage (GtkPrintOperation *operation,
                     GtkPrintContext   *context,
                     gint               page_nr);

    void OnEndPrint (GtkPrintOperation *operation,
                     GtkPrintContext   *context);

    static void OnPoolStatusUpdated (Pool           *pool,
                                     PoolSupervisor *ps);

    static gint CompareClassification (GtkTreeModel   *model,
                                       GtkTreeIter    *a,
                                       GtkTreeIter    *b,
                                       PoolSupervisor *pool_supervisor);

    static gint CompareSingleClassification (Player         *A,
                                             Player         *B,
                                             PoolSupervisor *pool_supervisor);

    static gint CompareCombinedClassification (Player         *A,
                                               Player         *B,
                                               PoolSupervisor *pool_supervisor);
};

#endif

