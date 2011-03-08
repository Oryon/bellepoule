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

#include "data.hpp"
#include "canvas_module.hpp"
#include "match.hpp"
#include "score_collector.hpp"
#include "stage.hpp"

class TableSet : public CanvasModule
{
  public:
    typedef void (*StatusCbk) (TableSet *table_set,
                               void     *data);
    TableSet (Stage     *supervisor,
              gchar     *id,
              GtkWidget *control_container);

    void SetStatusCbk (StatusCbk  cbk,
                       void      *data);

    void SetAttendees (GSList *attendees);

    void OnFromTableComboboxChanged ();
    void OnStuffClicked ();
    void OnInputToggled (GtkWidget *widget);
    void OnMatchSheetToggled (GtkWidget *widget);
    void OnDisplayToggled (GtkWidget *widget);
    void OnSearchMatch ();
    void OnPrint ();
    void OnZoom (gdouble value);
    void OnBeginPrint (GtkPrintOperation *operation,
                       GtkPrintContext   *context);
    void OnDrawPage (GtkPrintOperation *operation,
                     GtkPrintContext   *context,
                     gint               page_nr);

    void Wipe ();

    void Display ();

    void Lock ();

    void UnLock ();

    GSList *GetCurrentClassification ();

    void Load (xmlNode *xml_node);

    void Save (xmlTextWriter *xmlwriter);

  private:
    static const gdouble _score_rect_size;

    struct LevelStatus
    {
      gboolean       _has_error;
      guint          _is_over;
      GooCanvasItem *_status_item;
      GooCanvasItem *_level_header;
    };

    struct NodeData
    {
      guint          _expected_winner_rank;
      guint          _level;
      guint          _row;
      Match         *_match;
      GooCanvasItem *_canvas_table;
      GooCanvasItem *_player_item;
      GooCanvasItem *_print_item;
      GooCanvasItem *_connector;
    };

    static const gdouble _level_spacing;

    Stage              *_supervisor;
    GNode              *_tree_root;
    guint               _nb_levels;
    guint               _level_filter;
    GtkListStore       *_from_table_liststore;
    GtkTreeStore       *_quick_search_treestore;
    GtkTreeModelFilter *_quick_search_filter;
    guint               _nb_level_to_display;
    GooCanvasItem      *_main_table;
    GooCanvasItem      *_quick_score_A;
    GooCanvasItem      *_quick_score_B;
    Data               *_max_score;
    ScoreCollector     *_score_collector;
    ScoreCollector     *_quick_score_collector;
    xmlTextWriter      *_xml_writer;
    xmlNode            *_xml_node;
    LevelStatus        *_level_status;
    GSList             *_result_list;
    GSList             *_match_to_print;
    GtkWidget          *_print_dialog;
    GtkWidget          *_level_print_dialog;
    GtkWidget          *_control_container;
    GtkWidget          *_from_widget;
    gboolean            _print_full_table;
    gdouble             _print_scale;
    guint               _print_nb_x_pages;
    guint               _print_nb_y_pages;
    GData              *_match_list;
    GSList             *_attendees;
    gboolean            _locked;
    guint               _nb_match_per_sheet;
    gchar              *_defeated_table;
    gchar              *_id;

    void      *_status_cbk_data;
    StatusCbk  _status_cbk;

    GooCanvasItem *GetQuickScore (const gchar *container);

    void OnPlugged ();

    void OnUnPlugged ();

    void CreateTree ();

    void DeleteTree ();

    void OnAttrListUpdated ();

    void DrawAllConnectors ();

    void Garnish ();

    gboolean IsOver ();

    void LoadMatch (xmlNode *xml_node,
                    Match   *match);

    void LoadScore (xmlNode *xml_node,
                    Match   *match,
                    guint    player_index,
                    Player  **dropped);

    void OnStatusChanged (GtkComboBox *combo_box);

    static gboolean Stuff (GNode    *node,
                           TableSet *table_set);

    static gboolean AddToClassification (GNode    *node,
                                         TableSet *table_set);

    static gboolean UpdateLevelStatus (GNode    *node,
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

    static void OnNewScore (ScoreCollector *score_collector,
                            CanvasModule   *client,
                            Match          *match,
                            Player         *player);

    static gint ComparePlayer (Player    *A,
                               Player    *B,
                               TableSet  *table_set);

    gint ComparePreviousRankPlayer (Player  *A,
                                    Player  *B,
                                    guint32  rand_seed);

    void AddFork (GNode *to);

    void RefreshLevelStatus ();

    gchar *GetLevelImage (guint level);

    void SetPlayer (Match  *to_match,
                    Player *player,
                    guint   position);

    void LookForMatchToPrint (guint    level_to_print,
                              gboolean all_sheet);

    static void SetQuickSearchRendererSensitivity (GtkCellLayout   *cell_layout,
                                                   GtkCellRenderer *cell,
                                                   GtkTreeModel    *tree_model,
                                                   GtkTreeIter     *iter,
                                                   TableSet        *table_set);

    static gboolean OnPrintLevel (GooCanvasItem  *item,
                                  GooCanvasItem  *target_item,
                                  GdkEventButton *event,
                                  TableSet       *table_set);

    static gboolean OnPrintMatch (GooCanvasItem  *item,
                                  GooCanvasItem  *target_item,
                                  GdkEventButton *event,
                                  TableSet       *table_set);

    static void on_status_changed (GtkComboBox *combo_box,
                                   TableSet    *table_set);

    ~TableSet ();
};

#endif
