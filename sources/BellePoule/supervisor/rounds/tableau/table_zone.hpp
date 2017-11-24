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
#include <goocanvas.h>

#include "util/object.hpp"

class Match;
class CanvasModule;

namespace Table
{
  class Table;

  struct NodeData
  {
    guint          _expected_winner_rank;
    Table         *_table;
    guint          _table_index;
    Match         *_match;

    GooCanvasItem *_match_goo_table;
    GooCanvasItem *_fencer_goo_table;
    GooCanvasItem *_score_goo_table;
    GooCanvasItem *_score_goo_rect;
    GooCanvasItem *_score_goo_text;
    GooCanvasItem *_score_goo_image;
    GooCanvasItem *_fencer_goo_image;
    GooCanvasItem *_print_goo_icon;
    GooCanvasItem *_connector;
  };

  class TableZone : public Object
  {
    public:
      TableZone ();

      void AddNode (GNode *node);

      void PutRoadmapInTable (CanvasModule  *canvas_module,
                              GooCanvasItem *table,
                              guint          row,
                              guint          column);

    private:
      GNode *_node;

      virtual ~TableZone ();
  };
}
