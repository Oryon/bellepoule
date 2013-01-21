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
#include <goocanvas.h>

#include "object.hpp"
#include "module.hpp"
#include "referee_zone.hpp"

namespace Table
{
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

  class TableZone : public RefereeZone
  {
    public:
      TableZone (Module  *container,
                 gdouble  spacing);

      void AddReferee (Player *referee);

      void RemoveReferee (Player *referee);

      void AddNode (GNode *node);

      void Draw (GooCanvasItem *root_item);

      void PutInTable (CanvasModule  *canvas_module,
                       GooCanvasItem *table,
                       guint          row,
                       guint          column);

      GSList *GetNodeList ();

    private:
      GSList   *_node_list;
      gdouble   _spacing;

      virtual ~TableZone ();

      guint GetNbMatchs ();
  };
}

#endif
