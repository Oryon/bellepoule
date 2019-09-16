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

#include "util/module.hpp"
#include "../../stage.hpp"
#include "table_set.hpp"

class Error;
class Data;
class Match;
class ScoreCollector;

namespace Table
{
  class Table;

  class Supervisor : public Stage,
                     public Module,
                     public TableSet::Listener
  {
    public:
      static void Declare ();

      Supervisor (StageClass *stage_class);

      void OnStuffClicked ();
      void OnPreviousClicked ();
      void OnNextClicked ();
      void OnInputToggled (GtkWidget *widget);
      void OnPrintTableSet ();
      void OnPrintTableScoreSheets ();
      void OnSearchMatch (GtkEditable *editable);
      void OnTableSetTreeViewCursorChanged (GtkTreeView *treeview);
      void OnTableOver (TableSet *table_set,
                        Table    *table);

      void SaveHeader (XmlScheme *xml_scheme);

    public:
      static const gchar *_class_name;
      static const gchar *_xml_class_name;

    private:
      void OnLocked () override;
      void OnUnLocked () override;
      void OnPlugged () override;
      void OnUnPlugged () override;

    private:
      static const guint NONE         = 0;
      static const guint QUOTA        = 1;
      static const guint THIRD_PLACES = 3;
      static const guint ALL_PLACES   = 99;

      GtkTreeStore       *_table_set_treestore;
      GtkTreeModelFilter *_table_set_filter;
      xmlNode            *_xml_node;
      TableSet           *_displayed_table_set;
      gboolean            _is_over;
      Error::Provider    *_first_error;
      GSList             *_result;
      GSList             *_blackcardeds;
      Data               *_fenced_places;

      void Display () override;

      void Garnish () override;

      void CreateTableSets ();

      void DeleteTableSets ();

      void SetTableSetsState ();

      void ShowTableSet (TableSet    *table_set,
                         GtkTreeIter *iter);

      void HideTableSet (TableSet    *table_set,
                         GtkTreeIter *iter);

      void OnAttrListUpdated () override;

      gboolean OnMessage (Net::Message *message) override;

      gboolean IsOver () override;

      Error *GetError () override;

      GSList *GetCurrentClassification () override;

      static Stage *CreateInstance (StageClass *stage_class);

      TableSet *GetTableSet (gchar *id);

      void Recall () override;

      void Save (XmlScheme *xml_scheme) override;

      static gboolean SaveTableSet (GtkTreeModel *model,
                                    GtkTreePath  *path,
                                    GtkTreeIter  *iter,
                                    XmlScheme    *xml_scheme);

      void SaveConfiguration (XmlScheme *xml_scheme) override;

      void Load (xmlNode *xml_node) override;

      void LoadConfiguration (xmlNode *xml_node) override;

      gboolean OnHttpPost (const gchar *command,
                           const gchar **ressource,
                           const gchar *data);

      void DumpToHTML (FILE *file) override;

      void FillInConfig () override;

      void ApplyConfig () override;

      gboolean TableSetIsFenced (TableSet *table_set);

      void FeedTableSetStore (guint        from_place,
                              guint        nb_tables,
                              GtkTreeIter *parent);

      void OnTableSetSelected (TableSet *table_set);

      gchar *GetPrintName () override;

      GList *GetBookSections (StageView view) override;

      guint PreparePrint (GtkPrintOperation *operation,
                          GtkPrintContext   *context) override;

      void DrawPage (GtkPrintOperation *operation,
                     GtkPrintContext   *context,
                     gint               page_nr) override;

      void DrawConfig (GtkPrintOperation *operation,
                       GtkPrintContext   *context,
                       gint               page_nr) override;

      static gboolean ProcessMessage (GtkTreeModel    *model,
                                      GtkTreePath     *path,
                                      GtkTreeIter     *iter,
                                      Net::Message    *message);

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

      static gboolean ToggleTableSetLock (GtkTreeModel *model,
                                          GtkTreePath  *path,
                                          GtkTreeIter  *iter,
                                          gboolean      data);

      static gboolean ActivateTableSet (GtkTreeModel *model,
                                        GtkTreePath  *path,
                                        GtkTreeIter  *iter,
                                        Supervisor   *ts);

      static gboolean SpreadTableSet (GtkTreeModel *model,
                                      GtkTreePath  *path,
                                      GtkTreeIter  *iter,
                                      Supervisor   *ts);

      static gboolean RecallTableSet (GtkTreeModel *model,
                                      GtkTreePath  *path,
                                      GtkTreeIter  *iter,
                                      Supervisor   *ts);

      void OnTableSetStatusUpdated (TableSet *table_set) override;

      void OnTableSetDisplayed (TableSet *table_set,
                                Table    *from) override;

      ~Supervisor () override;
  };
}
