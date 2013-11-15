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

#include "util/data.hpp"
#include "util/module.hpp"
#include "common/match.hpp"
#include "common/score_collector.hpp"
#include "common/stage.hpp"

namespace Table
{
  class TableSet;
  class Table;

  class Supervisor : public virtual Stage, public Module
  {
    public:
      static void Declare ();

      Supervisor (StageClass *stage_class);

      void OnStuffClicked ();
      void OnInputToggled (GtkWidget *widget);
      void OnDisplayToggled (GtkWidget *widget);
      void OnPrint ();
      void OnZoom (gdouble value);
      void OnTableSetTreeViewCursorChanged (GtkTreeView *treeview);
      void OnTableOver (TableSet *table_set,
                        Table    *table);

    public:
      static const gchar *_class_name;
      static const gchar *_xml_class_name;

    private:
      void OnLocked ();
      void OnUnLocked ();
      void OnPlugged ();
      void OnUnPlugged ();

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
      Match              *_first_error;
      GSList             *_result;
      GSList             *_blackcardeds;
      Data               *_fenced_places;

      void Display ();

      void Garnish ();

      void CreateTableSets ();

      void DeleteTableSets ();

      void SetTableSetsState ();

      void ShowTableSet (TableSet    *table_set,
                         GtkTreeIter *iter);

      void HideTableSet (TableSet    *table_set,
                         GtkTreeIter *iter);

      void OnAttrListUpdated ();

      gboolean IsOver ();

      gchar *GetError ();

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

      gboolean OnHttpPost (const gchar *command,
                           const gchar **ressource,
                           const gchar *data);

        void FillInConfig ();

      void ApplyConfig ();

      gboolean TableSetIsFenced (TableSet *table_set);

      void FeedTableSetStore (guint        from_place,
                              guint        nb_tables,
                              GtkTreeIter *parent);

      void OnTableSetSelected (TableSet *table_set);

      gchar *GetPrintName ();

      guint PreparePrint (GtkPrintOperation *operation,
                          GtkPrintContext   *context);

      void DrawPage (GtkPrintOperation *operation,
                     GtkPrintContext   *context,
                     gint               page_nr);

      static gboolean TableSetIsOver (GtkTreeModel    *model,
                                      GtkTreePath     *path,
                                      GtkTreeIter     *iter,
                                      Supervisor *ts);

      static gboolean GetTableSetClassification (GtkTreeModel *model,
                                                 GtkTreePath  *path,
                                                 GtkTreeIter  *iter,
                                                 Supervisor   *ts);

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

      static gboolean ActivateTableSet (GtkTreeModel *model,
                                        GtkTreePath  *path,
                                        GtkTreeIter  *iter,
                                        Supervisor   *ts);

      static void OnTableSetStatusUpdated (TableSet   *table_set,
                                           Supervisor *ts);

      virtual ~Supervisor ();
  };
}

#endif
