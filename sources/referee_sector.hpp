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

#ifndef referee_sector_hpp
#define referee_sector_hpp

#include <gtk/gtk.h>

#include "object.hpp"

struct NodeData
{
  guint          _expected_winner_rank;
  Table         *_table;
  guint          _table_index;
  Match         *_match;
  GooCanvasItem *_fencer_goo_table;
  GooCanvasItem *_match_goo_table;
  GooCanvasItem *_connector;
};

class Player;

class RefereeSector : public Object
{
  public:
    RefereeSector (gdouble spacing);

    void AddNode (GNode *node);

    void Draw (GooCanvasItem *root_item);

    void Wipe ();

    void GetBounds (GooCanvasBounds *bounds,
                    gdouble          zoom_factor);

    void Focus ();

    void Unfocus ();

    void AddReferee (Player *referee);

    void RemoveReferee (Player *referee);

    guint GetNbMatchs ();

    void PutInTable (GooCanvasItem *table,
                     guint          row,
                     guint          column);

  private:
    GSList        *_node_list;
    GooCanvasItem *_back_rect;
    gdouble        _spacing;

    virtual ~RefereeSector ();

    static gboolean OnEnterNotify  (GooCanvasItem    *item,
                                    GooCanvasItem    *target_item,
                                    GdkEventCrossing *event,
                                    RefereeSector    *sector);

    static gboolean OnLeaveNotify  (GooCanvasItem    *item,
                                    GooCanvasItem    *target_item,
                                    GdkEventCrossing *event,
                                    RefereeSector    *sector);
};

#endif
