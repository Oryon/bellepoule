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

#include "util/canvas_module.hpp"
#include "network/ring.hpp"
#include "../../stage.hpp"
#include "../../score_collector.hpp"

#include "html_table.hpp"

class Error;
class Data;
class Match;
class ScoreCollector;

namespace Table
{
  class Supervisor;
  class Table;
  class HtmlTable;
  class PrintSession;
  class SheetCompositor;

  class TableSet :
    public CanvasModule,
    public ScoreCollector::Listener,
    public Net::Ring::PartnerListener
  {
    public:
      struct Listener
      {
        virtual void OnTableSetStatusUpdated (TableSet *table_set) = 0;
        virtual void OnTableSetDisplayed     (TableSet *table_set,
                                              Table    *from) = 0;
      };

    public:
      TableSet (Supervisor *supervisor,
                gchar      *id,
                guint       first_place,
                GtkRange   *zoomer);

      void SetListener (Listener *listener);

      void SetAttendees (GSList *attendees);

      void SetAttendees (GSList *attendees,
                         GSList *withdrawals);

      gboolean HasAttendees ();

      void OnBoutSheetChanged ();

      void OnStuffClicked ();

      void OnDisplayPrevious ();

      void OnDisplayNext ();

      void OnInputToggled (GtkWidget *widget);

      void OnMatchSheetToggled (GtkWidget *widget);

      void OnSearchMatch ();

      gboolean OnGotoMatch (gint number);

      void OnPrint ();

      void OnPrintScoreSheets (Table *table = NULL);

      GList *GetBookSections (Stage::StageView view);

      gchar *GetPrintName () override;

      guint PreparePrint (GtkPrintOperation *operation,
                          GtkPrintContext   *context) override;

      void DrawPage (GtkPrintOperation *operation,
                     GtkPrintContext   *context,
                     gint               page_nr) override;

      void Wipe () override;

      void Display ();

      void Lock ();

      void UnLock ();

      gboolean IsOver ();

      gboolean HasError ();

      GSList *GetCurrentClassification ();

      void Load (xmlNode *xml_node);

      void SaveHeader (XmlScheme *xml_scheme);

      void Save (XmlScheme *xml_scheme);

      void DumpToHTML (FILE *file) override;

      void SetPlayerToMatch (Match  *to_match,
                             Player *player,
                             guint   position);

      void RemovePlayerFromMatch (Match *to_match,
                                  guint  position);

      Stage *GetStage ();

      guint GetNbTables ();

      gchar *GetName ();

      guint GetFirstPlace ();

      gchar *GetId ();

      void Activate ();

      void DeActivate ();

      GSList *GetBlackcardeds ();

      Error::Provider *GetFirstError ();

      void Recall () override;

      gboolean OnMessage (Net::Message *message) override;

      gboolean OnHttpPost (const gchar *command,
                           const gchar **ressource,
                           const gchar *data);

    private:
      static const gdouble _score_rect_w;
      static const gdouble _score_rect_h;

      static const gdouble _table_spacing;

      gchar               *_short_name;
      Supervisor          *_supervisor;
      GNode               *_tree_root;
      guint                _nb_tables;
      GtkTreeStore        *_quick_search_treestore;
      GtkTreeModelFilter  *_quick_search_filter;
      GooCanvasItem       *_main_table;
      GooCanvasItem       *_quick_score_A;
      GooCanvasItem       *_quick_score_B;
      Data                *_max_score;
      ScoreCollector      *_score_collector;
      ScoreCollector      *_quick_score_collector;
      xmlNode             *_xml_node;
      Table              **_tables;
      GSList              *_result_list;
      SheetCompositor     *_sheet_compositor;
      GSList              *_attendees;
      GSList              *_withdrawals;
      gboolean             _locked;
      gchar               *_id;
      Error::Provider     *_first_error;
      gboolean             _is_over;
      gboolean             _loaded;
      guint                _first_place;
      gboolean             _is_active;
      GtkPageSetup        *_page_setup;
      Table               *_from_table;
      Table               *_to_table;
      gboolean            *_row_filled;
      HtmlTable           *_html_table;
      GdkPixbuf           *_printer_pixbuf;
      Filter              *_right_filter;
      GooCanvasItem       *_last_search;

      Listener *_listener;

      GooCanvasItem *GetQuickScore (const gchar *container);

      void OnPlugged () override;

      void OnUnPlugged () override;

      void CreateTree ();

      void DeleteTree ();

      void DrawAllConnectors ();

      void Garnish ();

      Table *GetTable (guint size);

      void LoadNode (xmlNode *xml_node);

      void OnStatusChanged (GtkComboBox *combo_box);

      void DrawPlayerMatch (GooCanvasItem *table,
                            Match         *match,
                            Player        *player,
                            guint          row);

      void OnPartnerJoined (Net::Partner *partner,
                            gboolean      joined) override;

      static gboolean Stuff (GNode    *node,
                             TableSet *table_set);

      static gboolean StartClassification (GNode    *node,
                                           TableSet *table_set);

      static gboolean CloseClassification (GNode    *node,
                                           TableSet *table_set);

      static gboolean UpdateTableStatus (GNode    *node,
                                         TableSet *table_set);

      static gboolean DrawConnector (GNode    *node,
                                     TableSet *table_set);

      static gboolean WipeNode (GNode    *node,
                                TableSet *table_set);

      static gboolean DeleteCanvasTable (GNode    *node,
                                         TableSet *table_set);

      static gboolean FillInNode (GNode    *node,
                                  TableSet *table_set);

      static gboolean DeleteNode (GNode    *node,
                                  TableSet *table_set);

      static gboolean DeleteDeadNode (GNode    *node,
                                      TableSet *table_set);

      void OnNewScore (ScoreCollector *score_collector,
                       Match          *match,
                       Player         *player) override;

      gint ComparePreviousRankPlayer (Player  *A,
                                      Player  *B,
                                      guint32  rand_seed);

      void AddFork (GNode *to);

      void RefreshTableStatus (gboolean quick = FALSE);

      void DropMatch (GNode *node);

      void LookForMatchToPrint (Table    *table_to_print,
                                gboolean  all_sheet);

      gboolean PlaceIsFenced (guint place);

      void ChangeFromTable (Table *table);

      PrintSession *GetPrintSession (GtkPrintOperation *operation);

      static gint ComparePlayer (Player   *A,
                                 Player   *B,
                                 TableSet *table_set);

      static void SetQuickSearchRendererSensitivity (GtkCellLayout   *cell_layout,
                                                     GtkCellRenderer *cell,
                                                     GtkTreeModel    *tree_model,
                                                     GtkTreeIter     *iter,
                                                     TableSet        *table_set);

      static gboolean OnPrintItemClicked (GooCanvasItem  *item,
                                          GooCanvasItem  *target_item,
                                          GdkEventButton *event,
                                          TableSet       *table_set);

      static gboolean OnPrintMatch (GooCanvasItem  *item,
                                    GooCanvasItem  *target_item,
                                    GdkEventButton *event,
                                    TableSet       *table_set);

      static void on_status_changed (GtkComboBox *combo_box,
                                     TableSet    *table_set);

      static gboolean on_status_key_press_event (GtkWidget   *widget,
                                                 GdkEventKey *event,
                                                 gpointer     user_data);

      static gboolean on_status_arrow_press (GooCanvasItem  *item,
                                             GooCanvasItem  *target,
                                             GdkEventButton *event,
                                             TableSet       *table_set);

      ~TableSet () override;

    private:
      GtkPrintOperationPreview *_preview;
      GtkWidget                *_current_preview_area;
      static const guint        PREVIEW_PAGE_SIZE = 200;

      void ConfigurePreviewBackground (GtkPrintOperation *operation,
                                       GtkPrintContext   *context);

      gboolean PreparePreview (GtkPrintOperation        *operation,
                               GtkPrintOperationPreview *preview,
                               GtkPrintContext          *context,
                               GtkWindow                *parent) override;

      void OnPreviewGotPageSize (GtkPrintOperationPreview *preview,
                                 GtkPrintContext          *context,
                                 GtkPageSetup             *page_setup) override;

      void OnPreviewReady (GtkPrintOperationPreview *preview,
                           GtkPrintContext          *context) override;

      static gboolean on_preview_expose (GtkWidget      *drawing_area,
                                         GdkEventExpose *event,
                                         TableSet       *ts);

    private:
      void GetBounds (GNode           *top,
                      GNode           *bottom,
                      GooCanvasBounds *bounds);

      void RefreshNodes ();

      static gboolean RefreshNode (GNode    *node,
                                   TableSet *table_set);

      void SpreadWinners ();

      void RefilterQuickSearch ();

      static gboolean SpreadWinner (GNode    *node,
                                    TableSet *table_set);

      void DumpNode (GNode *node);
  };
}
