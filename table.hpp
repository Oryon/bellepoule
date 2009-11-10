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

#include "stage.hpp"

class Table : public virtual Stage_c, public CanvasModule_c
{
  public:
    static void Init ();

    Table (StageClass *stage_class);

    void Save (xmlTextWriter *xml_writer);

  public:
    static const gchar *_class_name;
    static const gchar *_xml_class_name;

  private:
    void Enter ();
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
    };

    GNode         *_tree_root;
    guint          _nb_levels;
    GSList        *_attendees;
    GooCanvasItem *_main_table;
    guint          _max_score;

    void Display ();

    void OnPlugged ();

    void OnAttrListUpdated ();

    void DrawAllConnectors ();

    static gboolean DrawConnectors (GNode *node,
                                    Table *table);

    static Stage_c *CreateInstance (StageClass *stage_class);

    static gboolean FillInNode (GNode *node,
                                Table *table);

    void Load (xmlNode *xml_node);

    void AddFork (GNode *to);

    ~Table ();
};

#endif
