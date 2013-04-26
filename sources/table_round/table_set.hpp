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

#ifndef table_set_hpp
#define table_set_hpp

#include <gtk/gtk.h>

#include "util/data.hpp"
#include "util/canvas_module.hpp"
#include "util/drop_zone.hpp"
#include "common/match.hpp"
#include "common/score_collector.hpp"

#include "table.hpp"
#include "table_set_border.hpp"
#include "table_print_session.hpp"

namespace Table
{
  class Supervisor;

  class TableSet : public CanvasModule
  {
    public:
      typedef void (*StatusCbk) (TableSet *table_set,
                                 void     *data);

      TableSet (Supervisor *supervisor,
                gchar      *id,
                GtkWidget  *from_container,
                GtkWidget  *to_container,
                guint       first_place);

      void SetStatusCbk (StatusCbk  cbk,
                         void      *data);

      void SetAttendees (GSList *attendees);

      void SetAttendees (GSList *attendees,
                         GSList *withdrawals);

      gboolean HasAttendees ();

      void OnFromToTableComboboxChanged ();

      void OnCuttingCountComboboxChanged ();

      void OnStuffClicked ();

      void OnInputToggled (GtkWidget *widget);

      void OnMatchSheetToggled (GtkWidget *widget);

      void OnDisplayToggled (GtkWidget *widget);

      void OnSearchMatch ();

      void OnPrint ();

      gchar *GetPrintName ();

      guint PreparePrint (GtkPrintOperation *operation,
                          GtkPrintContext   *context);

      void DrawPage (GtkPrintOperation *operation,
                     GtkPrintContext   *context,
                     gint               page_nr);

      void OnPreviewClicked ();

      gboolean OnPreview (GtkPrintOperation        *operation,
                          GtkPrintOperationPreview *preview,
                          GtkPrintContext          *context,
                          GtkWindow                *parent);

      void OnPreviewGotPageSize (GtkPrintOperationPreview *preview,
                                 GtkPrintContext          *context,
                                 GtkPageSetup             *page_setup);

      void OnPreviewReady (GtkPrintOperationPreview *preview,
                           GtkPrintContext          *context);

      void OnPrinScaleChanged (gdouble value);

      void OnPageSetupClicked (GtkButton *button);

      void Wipe ();

      void Display ();

      void Lock ();

      void UnLock ();

      gboolean IsOver ();

      gboolean HasError ();

      GSList *GetCurrentClassification ();

      void Load (xmlNode *xml_node);

      void Save (xmlTextWriter *xmlwriter);

      void SetPlayerToMatch (Match  *to_match,
                             Player *player,
                             guint   position);

      Player *GetFencerFromRef (guint ref);

      void AddReferee (Match *match,
                       guint  referee_ref);

      guint GetNbTables ();

      gchar *GetName ();

      guint GetFirstPlace ();

      gchar *GetId ();

      void Activate ();

      void DeActivate ();

      GSList *GetBlackcardeds ();

      Match *GetFirstError ();

    private:
      static const gdouble _score_rect_size;

      static const gdouble _table_spacing;

      gchar                    *_short_name;
      Supervisor               *_supervisor;
      GNode                    *_tree_root;
      guint                     _nb_tables;
      guint                     _nb_matchs;
      gint                      _table_to_stuff;
      GtkTreeStore             *_quick_search_treestore;
      GtkTreeModelFilter       *_quick_search_filter;
      GooCanvasItem            *_main_table;
      GooCanvasItem            *_quick_score_A;
      GooCanvasItem            *_quick_score_B;
      Data                     *_max_score;
      ScoreCollector           *_score_collector;
      ScoreCollector           *_quick_score_collector;
      xmlTextWriter            *_xml_writer;
      xmlNode                  *_xml_node;
      Table                    **_tables;
      GSList                   *_result_list;
      GSList                   *_match_to_print;
      GtkWidget                *_print_dialog;
      GtkWidget                *_preview_dialog;
      GtkWidget                *_table_print_dialog;
      PrintSession              _print_session;
      GSList                   *_attendees;
      GSList                   *_withdrawals;
      gboolean                  _locked;
      guint                     _nb_match_per_sheet;
      gchar                    *_id;
      Match                    *_first_error;
      gboolean                  _is_over;
      gboolean                  _loaded;
      guint                     _first_place;
      GtkPrintOperationPreview *_preview;
      GtkWidget                *_current_preview_area;
      gboolean                  _is_active;
      GtkPageSetup             *_page_setup;
      TableSetBorder           *_from_border;
      TableSetBorder           *_to_border;
      gboolean                 *_row_filled;

      void      *_status_cbk_data;
      StatusCbk  _status_cbk;

      GooCanvasItem *GetQuickScore (const gchar *container);

      void OnPlugged ();

      void OnUnPlugged ();

      void CreateTree ();

      void DeleteTree ();

      void OnAttrListUpdated ();

      void DrawAllConnectors ();

      void DrawAllZones ();

      void Garnish ();

      Table *GetTable (guint size);

      void LoadNode (xmlNode *xml_node);

      void OnStatusChanged (GtkComboBox *combo_box);

      void DrawPlayerMatch (GooCanvasItem *table,
                            Match         *match,
                            Player        *player,
                            guint          row);

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

      static void OnNewScore (ScoreCollector *score_collector,
                              CanvasModule   *client,
                              Match          *match,
                              Player         *player);

      gint ComparePreviousRankPlayer (Player  *A,
                                      Player  *B,
                                      guint32  rand_seed);

      void AddFork (GNode *to);

      void RefreshTableStatus ();

      void DropMatch (GNode *node);

      void LookForMatchToPrint (Table    *table_to_print,
                                gboolean  all_sheet);

      void ConfigurePreviewLayout (GtkPrintContext *context);

      gboolean PlaceIsFenced (guint place);

      static gint ComparePlayer (Player    *A,
                                 Player    *B,
                                 TableSet  *table_set);

      static void SetQuickSearchRendererSensitivity (GtkCellLayout   *cell_layout,
                                                     GtkCellRenderer *cell,
                                                     GtkTreeModel    *tree_model,
                                                     GtkTreeIter     *iter,
                                                     TableSet        *table_set);

      static gboolean OnPrintTable (GooCanvasItem  *item,
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

      static gboolean on_preview_expose (GtkWidget      *drawing_area,
                                         GdkEventExpose *event,
                                         TableSet       *ts);

      virtual ~TableSet ();

    private:
      void DragObject (Object   *object,
                       DropZone *from_zone);

      void DropObject (Object   *object,
                       DropZone *source_zone,
                       DropZone *target_zone);

      Object *GetDropObjectFromRef (guint32 ref);

      gboolean DroppingIsForbidden (Object *object);

      GString *GetFloatingImage (Object *floating_object);

      gboolean ObjectIsDropable (Object   *floating_object,
                                 DropZone *in_zone);

      void GetBounds (GNode           *top,
                      GNode           *bottom,
                      GooCanvasBounds *bounds);
  };
}

#endif
