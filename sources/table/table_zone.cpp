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
#include "canvas_module.hpp"

#include "table_zone.hpp"

// --------------------------------------------------------------------------------
TableZone::TableZone (Module  *container,
                      gdouble  spacing)
: RefereeZone (container)
{
  _node_list = NULL;
  _spacing   = spacing;
}

// --------------------------------------------------------------------------------
TableZone::~TableZone ()
{
  while (_referee_list)
  {
    RemoveReferee ((Player *) _referee_list->data);
  }

  g_slist_free (_node_list);
}

// --------------------------------------------------------------------------------
void TableZone::AddNode (GNode *node)
{
  _node_list = g_slist_prepend (_node_list,
                                node);
}

// --------------------------------------------------------------------------------
GSList *TableZone::GetNodeList ()
{
  return _node_list;
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
                                          NULL);
      }
    }
    current = g_slist_next (current);
  }

  RefereeZone::Draw (root_item);
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

  RefereeZone::AddReferee (referee);
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

  RefereeZone::RemoveReferee (referee);
}

// --------------------------------------------------------------------------------
void TableZone::BookReferees ()
{
  GSList *current = _node_list;

  while (current)
  {
    GNode    *node = (GNode *) current->data;
    NodeData *data = (NodeData *) node->data;

    if (data->_match)
    {
      data->_match->BookReferees ();
    }
    current = g_slist_next (current);
  }
}

// --------------------------------------------------------------------------------
void TableZone::BookReferee (Player *referee)
{
  gboolean  booking_needed = FALSE;
  GSList   *current        = _node_list;

  while (current)
  {
    GNode    *node = (GNode *) current->data;
    NodeData *data = (NodeData *) node->data;

    if (data->_match && (data->_match->GetWinner () == NULL))
    {
      booking_needed = TRUE;
      break;
    }
    current = g_slist_next (current);
  }

  if (booking_needed)
  {
    RefereeZone::BookReferee (referee);
  }
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

// --------------------------------------------------------------------------------
void TableZone::PutInTable (CanvasModule  *canvas_module,
                            GooCanvasItem *table,
                            guint          row,
                            guint          column)
{
  GSList *current = _referee_list;

  if (current)
  {
    for (guint i = 0; current != NULL; i++)
    {
      Player *referee = (Player *) current->data;

      {
        static gchar *referee_icon = NULL;

        if (referee_icon == NULL)
        {
          referee_icon = g_build_filename (_program_path, "resources/glade/referee.png", NULL);
        }

        Canvas::PutIconInTable (table,
                                referee_icon,
                                row + i,
                                column);
      }

      {
        GooCanvasItem *item = Canvas::PutTextInTable (table,
                                                      referee->GetName (),
                                                      row + i,
                                                      column+1);
        Canvas::SetTableItemAttribute (item, "x-align", 1.0);
        Canvas::SetTableItemAttribute (item, "y-align", 0.5);
        g_object_set (item,
                      "font", "Sans Bold Italic 12px",
                      NULL);

        canvas_module->SetObjectDropZone (referee,
                                          item,
                                          this);
      }

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
