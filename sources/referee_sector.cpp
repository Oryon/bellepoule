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

#include "referee_sector.hpp"

// --------------------------------------------------------------------------------
RefereeSector::RefereeSector (gdouble spacing)
: Object ("RefereeSector")
{
  _node_list = NULL;
  _spacing   = spacing;

  Wipe ();
}

// --------------------------------------------------------------------------------
RefereeSector::~RefereeSector ()
{
  g_slist_free (_node_list);
}

// --------------------------------------------------------------------------------
void RefereeSector::Wipe ()
{
  _back_rect = NULL;
}

// --------------------------------------------------------------------------------
void RefereeSector::AddNode (GNode *node)
{
  _node_list = g_slist_prepend (_node_list,
                                node);
}

// --------------------------------------------------------------------------------
void RefereeSector::Draw (GooCanvasItem *root_item)
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
        goo_canvas_item_lower (_back_rect,
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
}

// --------------------------------------------------------------------------------
void RefereeSector::GetBounds (GooCanvasBounds *bounds,
                               gdouble          zoom_factor)
{
  if (_back_rect)
  {
    goo_canvas_item_get_bounds (_back_rect,
                                bounds);
    bounds->x1 *= zoom_factor;
    bounds->x2 *= zoom_factor;
    bounds->y1 *= zoom_factor;
    bounds->y2 *= zoom_factor;
  }
  else
  {
    bounds->x1 = 0;
    bounds->x2 = 0;
    bounds->y1 = 0;
    bounds->y2 = 0;
  }
}

// --------------------------------------------------------------------------------
void RefereeSector::Focus ()
{
  g_object_set (G_OBJECT (_back_rect),
                "fill-color", "Grey",
                NULL);
}

// --------------------------------------------------------------------------------
void RefereeSector::Unfocus ()
{
  g_object_set (G_OBJECT (_back_rect),
                "fill-color", "White",
                NULL);
}

// --------------------------------------------------------------------------------
void RefereeSector::AddReferee (Player *referee)
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
}

// --------------------------------------------------------------------------------
void RefereeSector::RemoveReferee (Player *referee)
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
}

// --------------------------------------------------------------------------------
guint RefereeSector::GetNbMatchs ()
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

// --------------------------------------------------------------------------------
void RefereeSector::PutInTable (GooCanvasItem *table,
                                guint          row,
                                guint          column)
{
  if (_node_list)
  {
    GNode    *node = (GNode *) _node_list->data;
    NodeData *data = (NodeData *) node->data;

    if (data->_match)
    {
      GSList *current = data->_match->GetRefereeList ();

      if (current)
      {
        for (guint i = 0; current != NULL; i++)
        {
          Player        *referee = (Player *) current->data;
          GooCanvasItem *item;
          static gchar  *referee_icon = NULL;

          if (referee_icon == NULL)
          {
            referee_icon = g_build_filename (_program_path, "resources/glade/referee.png", NULL);
          }

          Canvas::PutIconInTable (table,
                                  referee_icon,
                                  row + i,
                                  column);
          item = Canvas::PutTextInTable (table,
                                         referee->GetName (),
                                         row + i,
                                         column+1);
          Canvas::SetTableItemAttribute (item, "x-align", 1.0);
          Canvas::SetTableItemAttribute (item, "y-align", 0.5);
          g_object_set (item,
                        "font", "Sans Bold Italic 12px",
                        NULL);

          current = g_slist_next (current);
        }
      }
      else // Goocanvas display issue workaround
      {
        Canvas::PutTextInTable (table,
                                "",
                                row,
                                column+1);
      }
    }
  }
}

// --------------------------------------------------------------------------------
gboolean RefereeSector::OnEnterNotify  (GooCanvasItem    *item,
                                        GooCanvasItem    *target_item,
                                        GdkEventCrossing *event,
                                        RefereeSector    *sector)
{
  sector->Focus ();
  return TRUE;
}

// --------------------------------------------------------------------------------
gboolean RefereeSector::OnLeaveNotify  (GooCanvasItem    *item,
                                        GooCanvasItem    *target_item,
                                        GdkEventCrossing *event,
                                        RefereeSector    *sector)
{
  sector->Unfocus ();
  return TRUE;
}
