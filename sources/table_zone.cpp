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

#include "player.hpp"
#include "match.hpp"
#include "table.hpp"
#include "canvas.hpp"

#include "table_zone.hpp"

// --------------------------------------------------------------------------------
TableZone::TableZone (Module  *container,
                      gdouble  spacing)
: DropZone (container)
{
  _node_list = NULL;
  _spacing   = spacing;
}

// --------------------------------------------------------------------------------
TableZone::~TableZone ()
{
  g_slist_free (_node_list);
}

// --------------------------------------------------------------------------------
void TableZone::AddNode (GNode *node)
{
  _node_list = g_slist_prepend (_node_list,
                                node);
}

// --------------------------------------------------------------------------------
void TableZone::Draw (GooCanvasItem *root_item)
{
  GSList *current = _node_list;

  while (current)
  {
    GNode    *node = (GNode *) current->data;
    NodeData *data = (NodeData *) node->data;

    if (data->_table && data->_table->IsDisplayed () && data->_match)
    {
      GNode    *first_child      = g_node_first_child (node);
      GNode    *last_child       = g_node_last_child (node);
      NodeData *first_child_data = (NodeData *) first_child->data;
      NodeData *last_child_data  = (NodeData *) last_child->data;
      GooCanvasBounds first_child_bounds;
      GooCanvasBounds last_child_bounds;

      if (first_child_data->_fencer_goo_table && last_child_data->_fencer_goo_table)
      {
        goo_canvas_item_get_bounds (first_child_data->_fencer_goo_table,
                                    &first_child_bounds);
        goo_canvas_item_get_bounds (last_child_data->_fencer_goo_table,
                                    &last_child_bounds);

        _back_rect = goo_canvas_rect_new (root_item,
                                          first_child_bounds.x1,
                                          first_child_bounds.y1,
                                          first_child_bounds.x2 - first_child_bounds.x1,
                                          last_child_bounds.y2 - first_child_bounds.y1,
                                          "stroke-pattern", NULL,
                                          "pointer-events", GOO_CANVAS_EVENTS_VISIBLE,
                                          NULL);

#if 0
        g_signal_connect (_back_rect, "enter_notify_event",
                          G_CALLBACK (OnEnterNotify), this);
        g_signal_connect (_back_rect, "leave_notify_event",
                          G_CALLBACK (OnLeaveNotify), this);
#endif
      }
    }
    current = g_slist_next (current);
  }

  DropZone::Draw (root_item);
}

// --------------------------------------------------------------------------------
void TableZone::AddReferee (Player *referee)
{
  GSList *current = _node_list;

  while (current)
  {
    GNode    *node = (GNode *) current->data;
    NodeData *data = (NodeData *) node->data;

    if (data->_match)
    {
      data->_match->AddReferee (referee);
    }
    current = g_slist_next (current);
  }

  DropZone::AddReferee (referee);
}

// --------------------------------------------------------------------------------
void TableZone::RemoveReferee (Player *referee)
{
  GSList *current = _node_list;

  while (current)
  {
    GNode    *node = (GNode *) current->data;
    NodeData *data = (NodeData *) node->data;

    if (data->_match)
    {
      data->_match->RemoveReferee (referee);
    }
    current = g_slist_next (current);
  }

  DropZone::RemoveReferee (referee);
}

// --------------------------------------------------------------------------------
guint TableZone::GetNbMatchs ()
{
  GSList *current = _node_list;
  guint   result  = 0;

  while (current)
  {
    GNode    *node = (GNode *) current->data;
    NodeData *data = (NodeData *) node->data;

    if (data->_match)
    {
      result++;
    }
    current = g_slist_next (current);
  }

  return result;
}
