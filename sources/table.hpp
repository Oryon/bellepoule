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

#ifndef table_hpp
#define table_hpp

#include <gtk/gtk.h>

#include "data.hpp"
#include "canvas_module.hpp"
#include "match.hpp"
#include "score_collector.hpp"

#include "stage.hpp"

class Table : public virtual Stage, public CanvasModule
{
  public:
    static void Init ();

    Table (StageClass *stage_class);

    void OnFromTableComboboxChanged ();
    void OnStuffClicked ();
    void OnInputToggled (GtkWidget *widget);
    void OnMatchSheetToggled (GtkWidget *widget);
    void OnSearchMatch ();
    void OnFilterClicked ();
    void OnPrint ();
    void OnZoom (gdouble value);

  public:
    static const gchar *_class_name;
    static const gchar *_xml_class_name;

  private:
    void OnLocked (Reason reason);
    void OnUnLocked ();
    void OnPlugged ();
    void OnUnPlugged ();
    void Wipe ();

    void OnBeginPrint (GtkPrintOperation *operation,
                       GtkPrintContext   *context);
    void OnDrawPage (GtkPrintOperation *operation,
                     GtkPrintContext   *context,
                     gint               page_nr);

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
    gboolean            _print_full_table;
    gdouble             _print_scale;
    guint               _print_nb_x_pages;
    guint               _print_nb_y_pages;
    GData              *_match_list;

    GooCanvasItem *GetQuickScore (const gchar *container);

    void Display ();

    void Garnish ();

    void CreateTree ();

    void DeleteTree ();

    void OnAttrListUpdated ();

    void DrawAllConnectors ();

    gboolean IsOver ();

    void LoadMatch (xmlNode *xml_node,
                    Match   *match);

    void LoadScore (xmlNode *xml_node,
                    Match   *match,
                    guint    player_index,
                    Player  **dropped);

    GSList *GetCurrentClassification ();

    void OnStatusChanged (GtkComboBox *combo_box);

    static gboolean Stuff (GNode *node,
                           Table *table);

    static gboolean AddToClassification (GNode *node,
                                         Table *table);

    static gboolean UpdateLevelStatus (GNode *node,
                                       Table *table);

    static gboolean DrawConnector (GNode *node,
                                   Table *table);

    static gboolean WipeNode (GNode *node,
                              Table *table);

    static gboolean DeleteCanvasTable (GNode *node,
                                       Table *table);

    static Stage *CreateInstance (StageClass *stage_class);

    static gboolean FillInNode (GNode *node,
                                Table *table);

    static gboolean DeleteNode (GNode *node,
                                Table *table);

    static void OnNewScore (ScoreCollector *score_collector,
                            CanvasModule   *client,
                            Match          *match,
                            Player         *player);

    static gint ComparePlayer (Player *A,
                               Player *B,
                               Table  *table);

    gint ComparePreviousRankPlayer (Player  *A,
                                    Player  *B,
                                    guint32  rand_seed);

    void Save (xmlTextWriter *xml_writer);

    void SaveConfiguration (xmlTextWriter *xml_writer);

    void Load (xmlNode *xml_node);

    void LoadConfiguration (xmlNode *xml_node);

    void AddFork (GNode *to);

    void FillInConfig ();

    void ApplyConfig ();

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
                                                   Table           *table);

    static gboolean OnPrintLevel (GooCanvasItem  *item,
                                  GooCanvasItem  *target_item,
                                  GdkEventButton *event,
                                  Table          *table);

    static gboolean OnPrintMatch (GooCanvasItem  *item,
                                  GooCanvasItem  *target_item,
                                  GdkEventButton *event,
                                  Table          *table);

    static void on_status_changed (GtkComboBox *combo_box,
                                   Table       *table);

    ~Table ();
};

#endif
