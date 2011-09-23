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

#ifndef table_supervisor_hpp
#define table_supervisor_hpp

#include <gtk/gtk.h>

#include "data.hpp"
#include "module.hpp"
#include "match.hpp"
#include "score_collector.hpp"

#include "stage.hpp"

class TableSet;
class Table;

class TableSupervisor : public virtual Stage, public Module
{
  public:
    static void Init ();

    TableSupervisor (StageClass *stage_class);

    void OnStuffClicked ();
    void OnInputToggled (GtkWidget *widget);
    void OnDisplayToggled (GtkWidget *widget);
    void OnFilterClicked ();
    void OnPrint ();
    void OnZoom (gdouble value);
    void OnTableSetTreeViewCursorChanged (GtkTreeView *treeview);
    void OnTableOver (TableSet *table_set,
                      Table    *table);

  public:
    static const gchar *_class_name;
    static const gchar *_xml_class_name;

  private:
    void OnLocked (Reason reason);
    void OnUnLocked ();
    void OnPlugged ();
    void OnUnPlugged ();
    void Wipe ();

  private:
    static const guint NONE         = 0;
    static const guint QUOTA        = 1;
    static const guint THIRD_PLACES = 3;
    static const guint ALL_PLACES   = 99;

    GtkTreeStore       *_table_set_treestore;
    GtkTreeModelFilter *_table_set_filter;
    xmlTextWriter      *_xml_writer;
    xmlNode            *_xml_node;
    TableSet           *_displayed_table_set;
    gboolean            _is_over;
    GSList             *_result;
    GSList             *_blackcardeds;
    Data               *_fenced_places;

    void Display ();

    void Garnish ();

    void CreateTableSets ();

    void DeleteTableSets ();

    void SetTableSetsState ();

    void OnAttrListUpdated ();

    gboolean IsOver ();

    GSList *GetCurrentClassification ();

    static Stage *CreateInstance (StageClass *stage_class);

    TableSet *GetTableSet (gchar *id);

    void Save (xmlTextWriter *xml_writer);

    static gboolean SaveTableSet (GtkTreeModel  *model,
                                  GtkTreePath   *path,
                                  GtkTreeIter   *iter,
                                  xmlTextWriter *xml_writer);

    void SaveConfiguration (xmlTextWriter *xml_writer);

    void Load (xmlNode *xml_node);

    void LoadConfiguration (xmlNode *xml_node);

    void FillInConfig ();

    void ApplyConfig ();

    void FeedTableSetStore (guint        from_place,
                            guint        nb_tables,
                            GtkTreeIter *parent);

    void OnTableSetSelected (TableSet *table_set);

    static gboolean TableSetIsOver (GtkTreeModel    *model,
                                    GtkTreePath     *path,
                                    GtkTreeIter     *iter,
                                    TableSupervisor *ts);

    static gboolean GetTableSetClassification (GtkTreeModel    *model,
                                               GtkTreePath     *path,
                                               GtkTreeIter     *iter,
                                               TableSupervisor *ts);

    static gboolean DeleteTableSet (GtkTreeModel *model,
                                    GtkTreePath  *path,
                                    GtkTreeIter  *iter,
                                    gpointer      data);

    static gboolean StuffTableSet (GtkTreeModel *model,
                                   GtkTreePath  *path,
                                   GtkTreeIter  *iter,
                                   gpointer      data);

    static gboolean ToggleTableSetLock (GtkTreeModel *model,
                                        GtkTreePath  *path,
                                        GtkTreeIter  *iter,
                                        gboolean      data);

    static gboolean ActivateTableSet (GtkTreeModel    *model,
                                      GtkTreePath     *path,
                                      GtkTreeIter     *iter,
                                      TableSupervisor *ts);

    static void OnTableSetStatusUpdated (TableSet        *table_set,
                                         TableSupervisor *ts);
    ~TableSupervisor ();
};

#endif
