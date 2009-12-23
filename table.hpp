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

#include "canvas_module.hpp"
#include "match.hpp"
#include "score_collector.hpp"

#include "stage.hpp"

class Table : public virtual Stage_c, public CanvasModule_c
{
  public:
    static void Init ();

    Table (StageClass *stage_class);

    void OnFromTableComboboxChanged ();

  public:
    static const gchar *_class_name;
    static const gchar *_xml_class_name;

  private:
    void OnLocked ();
    void OnUnLocked ();
    void Wipe ();

  private:
    struct NodeData
    {
      guint          _expected_winner_rank;
      guint          _level;
      guint          _row;
      Match_c       *_match;
      GooCanvasItem *_canvas_table;
      GooCanvasItem *_player_item;
      GooCanvasItem *_print_item;
      GooCanvasItem *_connector;
    };

    static const gdouble _level_spacing;

    GNode          *_tree_root;
    guint           _nb_levels;
    GtkListStore   *_from_table_liststore;
    guint           _nb_level_to_display;
    GooCanvasItem  *_main_table;
    guint           _max_score;
    ScoreCollector *_score_collector;
    xmlTextWriter  *_xml_writer;
    xmlNode        *_xml_node;

    void Display ();

    void Garnish ();

    void CreateTree ();

    void DeleteTree ();

    void OnAttrListUpdated ();

    void DrawAllConnectors ();

    static gboolean DrawConnector (GNode *node,
                                   Table *table);

    static gboolean WipeNode (GNode *node,
                              Table *table);

    static gboolean DeleteCanvasTable (GNode *node,
                                       Table *table);

    static Stage_c *CreateInstance (StageClass *stage_class);

    static gboolean FillInNode (GNode *node,
                                Table *table);

    static gboolean DeleteNode (GNode *node,
                                Table *table);

    static gboolean SaveNode (GNode *node,
                              Table *table);

    static gboolean LoadNode (GNode *node,
                              Table *table);

    static void OnNewScore (CanvasModule_c *client,
                            Match_c        *match,
                            Player_c       *player);

    void Save (xmlTextWriter *xml_writer);

    void Load (xmlNode *xml_node);

    void AddFork (GNode *to);

    void FillInConfig ();

    void ApplyConfig ();

    ~Table ();
};

#endif
