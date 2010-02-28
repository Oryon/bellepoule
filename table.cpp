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

#include <string.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <gdk/gdkkeysyms.h>

#include "attribute.hpp"
#include "player.hpp"
#include "table.hpp"

const gchar   *Table::_class_name     = "table";
const gchar   *Table::_xml_class_name = "table_stage";
const gdouble  Table::_level_spacing  = 10.0;

typedef enum
{
  FROM_NAME_COLUMN,
  FROM_STATUS_COLUMN
} FromColumnId;

typedef enum
{
  QUICK_MATCH_NAME_COLUMN,
  QUICK_MATCH_COLUMN,
  QUICK_MATCH_PATH_COLUMN,
  QUICK_MATCH_VISIBILITY_COLUMN
} QuickSearchColumnId;

extern "C" G_MODULE_EXPORT void on_from_table_combobox_changed (GtkWidget *widget,
                                                                Object_c  *owner);

// --------------------------------------------------------------------------------
Table::Table (StageClass *stage_class)
: Stage_c (stage_class),
  CanvasModule_c ("table.glade")
{
  _main_table   = NULL;
  _tree_root    = NULL;
  _level_status = NULL;

  _score_collector = NULL;

  _max_score = 10;

  {
    AddSensitiveWidget (_glade->GetWidget ("input_toolbutton"));
    AddSensitiveWidget (_glade->GetWidget ("stuff_toolbutton"));

    LockOnClassification (_glade->GetWidget ("from_vbox"));
    LockOnClassification (_glade->GetWidget ("stuff_toolbutton"));
    LockOnClassification (_glade->GetWidget ("input_toolbutton"));
  }

  {
    GSList *attr_list;
    Filter *filter;

    AttributeDesc::CreateList (&attr_list,
                               "ref",
                               "attending",
                               "exported",
                               NULL);
    filter = new Filter (attr_list,
                         this);

    filter->ShowAttribute ("rank");
    filter->ShowAttribute ("name");
    filter->ShowAttribute ("club");

    SetFilter (filter);
    filter->Release ();
  }

  {
    _quick_score_collector = new ScoreCollector (this,
                                                 (ScoreCollector::OnNewScore_cbk) &Table::OnNewScore);

    _quick_score_A = GetQuickScore ("fencerA_hook");
    _quick_score_B = GetQuickScore ("fencerB_hook");

    _quick_score_collector->SetNextCollectingPoint (_quick_score_A,
                                                    _quick_score_B);
    _quick_score_collector->SetNextCollectingPoint (_quick_score_B,
                                                    _quick_score_A);
  }

  _from_table_liststore   = GTK_LIST_STORE (_glade->GetObject ("from_liststore"));
  _quick_search_treestore = GTK_TREE_STORE (_glade->GetObject ("match_treestore"));
  _quick_search_filter    = GTK_TREE_MODEL_FILTER (_glade->GetObject ("match_treemodelfilter"));

  {
    gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (_glade->GetWidget ("quick_search_combobox")),
                                        (GtkCellRenderer *) _glade->GetWidget ("quick_search_renderertext"),
                                        (GtkCellLayoutDataFunc) SetQuickSearchRendererSensitivity,
                                        this, NULL);
    gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (_glade->GetWidget ("quick_search_combobox")),
                                        (GtkCellRenderer *) _glade->GetWidget ("quick_search_path_renderertext"),
                                        (GtkCellLayoutDataFunc) SetQuickSearchRendererSensitivity,
                                        this, NULL);
    gtk_tree_model_filter_set_visible_column (_quick_search_filter,
                                              QUICK_MATCH_VISIBILITY_COLUMN);
  }
}

// --------------------------------------------------------------------------------
Table::~Table ()
{
  g_object_unref (_from_table_liststore);
  g_object_unref (_quick_search_treestore);
  g_object_unref (_quick_search_filter);
}

// --------------------------------------------------------------------------------
void Table::Init ()
{
  RegisterStageClass (_class_name,
                      _xml_class_name,
                      CreateInstance,
                      EDITABLE);
}

// --------------------------------------------------------------------------------
void Table::SetQuickSearchRendererSensitivity (GtkCellLayout   *cell_layout,
                                               GtkCellRenderer *cell,
                                               GtkTreeModel    *tree_model,
                                               GtkTreeIter     *iter,
                                               Table           *table)
{
  gboolean sensitive = !gtk_tree_model_iter_has_child (tree_model, iter);

  g_object_set (cell, "sensitive", sensitive, NULL);
}

// --------------------------------------------------------------------------------
GooCanvasItem *Table::GetQuickScore (gchar *container)
{
  GtkWidget     *view_port = _glade->GetWidget (container);
  GooCanvas     *canvas    = GOO_CANVAS (goo_canvas_new ());
  GooCanvasItem *goo_rect;
  GooCanvasItem *score_text;

  gtk_container_add (GTK_CONTAINER (view_port),
                     GTK_WIDGET (canvas));
  gtk_widget_show (GTK_WIDGET (canvas));

  goo_rect = goo_canvas_rect_new (goo_canvas_get_root_item (canvas),
                                  0, 0,
                                  _score_rect_size, _score_rect_size,
                                  "line-width", 0.0,
                                  "pointer-events", GOO_CANVAS_EVENTS_VISIBLE,
                                  NULL);

  score_text = goo_canvas_text_new (goo_canvas_item_get_parent (goo_rect),
                                    "",
                                    _score_rect_size/2, _score_rect_size/2,
                                    -1,
                                    GTK_ANCHOR_CENTER,
                                    "font", "Sans bold 18px",
                                    NULL);

  _quick_score_collector->AddCollectingPoint (goo_rect,
                                              score_text,
                                              NULL,
                                              NULL);

  return goo_rect;
}

// --------------------------------------------------------------------------------
Stage_c *Table::CreateInstance (StageClass *stage_class)
{
  return new Table (stage_class);
}

// --------------------------------------------------------------------------------
void Table::RefreshLevelStatus ()
{
  for (guint i = 0; i < _nb_levels; i ++)
  {
    _level_status[i]._is_over   = TRUE;
    _level_status[i]._has_error = FALSE;
    if (_level_status[i]._status_item)
    {
      WipeItem (_level_status[i]._status_item);
      _level_status[i]._status_item = NULL;
    }
  }

  g_node_traverse (_tree_root,
                   G_POST_ORDER,
                   G_TRAVERSE_ALL,
                   _nb_level_to_display - 1,
                   (GNodeTraverseFunc) UpdateLevelStatus,
                   this);

  {
    guint nb_missing_level = _nb_levels - _nb_level_to_display;

    for (guint i = nb_missing_level; i < _nb_levels - 1; i ++)
    {
      GtkTreeIter  iter;
      gchar       *icon;

      if (_level_status[i]._has_error)
      {
        icon = GTK_STOCK_DIALOG_WARNING;
      }
      else if (_level_status[i]._is_over == TRUE)
      {
        icon = GTK_STOCK_APPLY;
      }
      else
      {
        icon = GTK_STOCK_EXECUTE;
      }

      _level_status[i]._status_item = PutStockIconInTable (_level_status[i-nb_missing_level]._level_header,
                                                           icon,
                                                           0, 0);

      gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (_from_table_liststore),
                                     &iter,
                                     NULL,
                                     i-nb_missing_level);
      gtk_list_store_set (_from_table_liststore, &iter,
                          FROM_STATUS_COLUMN, icon,
                          -1);
    }
  }
  SignalStatusUpdate ();
}

// --------------------------------------------------------------------------------
void Table::Display ()
{
  Wipe ();

  {
    _main_table = goo_canvas_table_new (GetRootItem (),
                                        "row-spacing",    10.0,
                                        "column-spacing", _level_spacing,
                                        NULL);

    // Header
    for (guint l = _nb_level_to_display; l > 0; l--)
    {
      _level_status[l-1]._level_header = goo_canvas_table_new (_main_table,
                                                               NULL);
      {
        GooCanvasItem *text_item;
        gchar         *text = GetLevelImage (l + (_nb_levels - _nb_level_to_display));

        text_item = PutTextInTable (_level_status[l-1]._level_header,
                                    text,
                                    0,
                                    1);
        g_object_set (G_OBJECT (text_item),
                      "font", "Sans bold 18px",
                      NULL);
        g_free (text);
      }

      PutInTable (_main_table,
                  _level_status[l-1]._level_header,
                  0,
                  l);
      SetTableItemAttribute (_level_status[l-1]._level_header, "x-align", 0.5);
    }

    g_node_traverse (_tree_root,
                     G_POST_ORDER,
                     G_TRAVERSE_ALL,
                     _nb_level_to_display,
                     (GNodeTraverseFunc) FillInNode,
                     this);

    RefreshLevelStatus ();
    DrawAllConnectors ();
  }
}

// --------------------------------------------------------------------------------
gboolean Table::IsOver ()
{
  NodeData *root_data = (NodeData *) _tree_root->data;

  if (root_data->_match)
  {
    return (root_data->_match->GetWinner () != NULL);
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
void Table::Garnish ()
{
  DeleteTree ();
  CreateTree ();
}

// --------------------------------------------------------------------------------
void Table::LoadConfiguration (xmlNode *xml_node)
{
  Stage_c::LoadConfiguration (xml_node);

  {
    gchar *attr = (gchar *) xmlGetProp (xml_node,
                                        BAD_CAST "max_score");
    if (attr)
    {
      _max_score = (guint) atoi (attr);
    }
  }
}

// --------------------------------------------------------------------------------
void Table::Load (xmlNode *xml_node)
{
  LoadConfiguration (xml_node);

  if (_attendees)
  {
    CreateTree ();

    if (_tree_root)
    {
      if (xml_node->type == XML_ELEMENT_NODE)
      {
        if (strcmp ((char *) xml_node->name, _xml_class_name) == 0)
        {
          _xml_node = xml_node->children->next;

          g_node_traverse (_tree_root,
                           G_POST_ORDER,
                           G_TRAVERSE_ALL,
                           -1,
                           (GNodeTraverseFunc) LoadNode,
                           this);
        }
      }
    }
  }
}

// --------------------------------------------------------------------------------
void Table::SaveConfiguration (xmlTextWriter *xml_writer)
{
  Stage_c::SaveConfiguration (xml_writer);

  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "max_score",
                                     "%d", _max_score);
}

// --------------------------------------------------------------------------------
void Table::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST _xml_class_name);

  SaveConfiguration (xml_writer);

  if (_tree_root)
  {
    _xml_writer = xml_writer;
    g_node_traverse (_tree_root,
                     G_POST_ORDER,
                     G_TRAVERSE_ALL,
                     -1,
                     (GNodeTraverseFunc) SaveNode,
                     this);
  }

  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void Table::OnNewScore (ScoreCollector *score_collector,
                        CanvasModule_c *client,
                        Match_c        *match,
                        Player_c       *player)
{
  Table *table = dynamic_cast <Table *> (client);
  GNode *node  = (GNode *) match->GetData (table, "node");

  FillInNode (node,
              table);

  if (node->parent)
  {
    FillInNode (node->parent,
                table);
  }

  if (score_collector == table->_quick_score_collector)
  {
    table->_score_collector->Refresh (match);
  }
  else
  {
    table->_quick_score_collector->Refresh (match);
  }

  table->RefreshLevelStatus ();
  table->DrawAllConnectors ();
}

// --------------------------------------------------------------------------------
void Table::DeleteTree ()
{
  g_signal_handlers_disconnect_by_func (_glade->GetWidget ("from_table_combobox"),
                                        (void *) on_from_table_combobox_changed,
                                        (Object_c *) this);

  if (_tree_root)
  {
    g_node_traverse (_tree_root,
                     G_POST_ORDER,
                     G_TRAVERSE_ALL,
                     -1,
                     (GNodeTraverseFunc) DeleteNode,
                     this);

    g_node_destroy (_tree_root);
    _tree_root = NULL;
  }

  g_free (_level_status);
  _level_status = NULL;
}

// --------------------------------------------------------------------------------
void Table::CreateTree ()
{
  guint nb_players;

  nb_players = g_slist_length (_attendees);

  for (guint i = 0; i < 32; i++)
  {
    guint bit_cursor;

    bit_cursor = 1;
    bit_cursor = bit_cursor << i;
    if (bit_cursor >= nb_players)
    {
      _nb_levels = i++;
      break;
    }
  }

  _nb_levels++;
  _nb_level_to_display = _nb_levels;

  _level_status = (LevelStatus *) g_malloc0 (_nb_levels * sizeof (LevelStatus));

  AddFork (NULL);

  {
    g_signal_handlers_disconnect_by_func (_glade->GetWidget ("from_table_combobox"),
                                          (void *) on_from_table_combobox_changed,
                                          (Object_c *) this);
    gtk_list_store_clear (_from_table_liststore);

    for (guint i = 0; i < _nb_levels - 1; i++)
    {
      GtkTreeIter   iter;
      gchar        *text;

      gtk_list_store_append (_from_table_liststore,
                             &iter);

      text = GetLevelImage (i+1);
      gtk_list_store_set (_from_table_liststore, &iter,
                          FROM_STATUS_COLUMN, GTK_STOCK_EXECUTE,
                          FROM_NAME_COLUMN,   text,
                          -1);
      g_free (text);
    }

    gtk_combo_box_set_active (GTK_COMBO_BOX (_glade->GetWidget ("from_table_combobox")),
                              0);

    g_signal_connect (_glade->GetWidget ("from_table_combobox"), "changed",
                      G_CALLBACK (on_from_table_combobox_changed),
                      (Object_c *) this);
  }
}

// --------------------------------------------------------------------------------
void Table::DrawAllConnectors ()
{
  g_node_traverse (_tree_root,
                   G_POST_ORDER,
                   G_TRAVERSE_ALL,
                   _nb_level_to_display,
                   (GNodeTraverseFunc) DrawConnector,
                   this);
}

// --------------------------------------------------------------------------------
gboolean Table::WipeNode (GNode *node,
                          Table *table)
{
  NodeData *data = (NodeData *) node->data;

  WipeItem (data->_player_item);
  WipeItem (data->_print_item);
  WipeItem (data->_connector);

  data->_connector   = NULL;
  data->_player_item = NULL;
  data->_print_item  = NULL;

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean Table::DeleteCanvasTable (GNode *node,
                                   Table *table)
{
  NodeData *data = (NodeData *) node->data;

  data->_player_item = NULL;
  data->_print_item  = NULL;
  data->_connector   = NULL;

  data->_canvas_table = NULL;

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean Table::UpdateLevelStatus (GNode *node,
                                   Table *table)
{
  NodeData *data = (NodeData *) node->data;

  if (data->_match)
  {
    Player_c *winner = data->_match->GetWinner ();

    if (winner == NULL)
    {
      Player_c *A       = data->_match->GetPlayerA ();
      Player_c *B       = data->_match->GetPlayerB ();
      Score_c  *score_A = data->_match->GetScore (A);
      Score_c  *score_B = data->_match->GetScore (B);

      if (   (score_A->IsValid () == FALSE)
          || (score_B->IsValid () == FALSE)
          || (score_A->IsConsistentWith (score_B) == FALSE))
      {
        table->_level_status[data->_level-2]._has_error = TRUE;
      }

      table->_level_status[data->_level-2]._is_over = FALSE;
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean Table::DrawConnector (GNode *node,
                               Table *table)
{
  NodeData *data = (NodeData *) node->data;

  if (data->_match)
  {
    Player_c        *winner = data->_match->GetWinner ();
    GNode           *parent = node->parent;
    GooCanvasBounds  bounds;

    WipeItem (data->_connector);
    data->_connector = NULL;

    goo_canvas_item_get_bounds (data->_canvas_table,
                                &bounds);

    if (G_NODE_IS_ROOT (node))
    {
      data->_connector = goo_canvas_polyline_new (table->GetRootItem (),
                                                  FALSE,
                                                  2,
                                                  bounds.x1 - _level_spacing/2, bounds.y2,
                                                  bounds.x2, bounds.y2,
                                                  "line-width", 2.0,
                                                  NULL);
    }
    else if (   (G_NODE_IS_LEAF (node) == FALSE)
                || (winner))
    {
      NodeData        *parent_data = (NodeData *) parent->data;
      GooCanvasBounds  parent_bounds;

      goo_canvas_item_get_bounds (parent_data->_canvas_table,
                                  &parent_bounds);

      data->_connector = goo_canvas_polyline_new (table->GetRootItem (),
                                                  FALSE,
                                                  3,
                                                  bounds.x1 - _level_spacing/2, bounds.y2,
                                                  parent_bounds.x1 - _level_spacing/2, bounds.y2,
                                                  parent_bounds.x1 - _level_spacing/2, parent_bounds.y2,
                                                  "line-width", 2.0,
                                                  NULL);
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean Table::FillInNode (GNode *node,
                            Table *table)
{
  GNode    *parent           = node->parent;
  NodeData *data             = (NodeData *) node->data;
  guint     nb_missing_level = table->_nb_levels - table->_nb_level_to_display;

  if (data->_match && (data->_level > nb_missing_level))
  {
    Player_c *winner = data->_match->GetWinner ();

    WipeNode (node,
              table);

    if (data->_canvas_table == NULL)
    {
      guint row;

      data->_canvas_table = goo_canvas_table_new (table->_main_table,
                                                  "column-spacing", table->_level_spacing,
                                                  NULL);
      SetTableItemAttribute (data->_canvas_table, "x-expand", 1U);
      SetTableItemAttribute (data->_canvas_table, "x-fill", 1U);

      if (parent == NULL)
      {
        row = 1 << (table->_nb_level_to_display - 1);
      }
      else
      {
        if (g_node_child_position (parent,
                                   node) == 0)
        {
          row = data->_row >> nb_missing_level;
        }
        else
        {
          row = data->_row >> nb_missing_level;
        }
      }
      PutInTable (table->_main_table,
                  data->_canvas_table,
                  row,
                  data->_level - nb_missing_level);
    }

    {
      guint number = (guint) data->_match->GetData (table, "number");

      if (number)
      {
        gchar         *text        = g_strdup_printf ("No %d", number);
        GooCanvasItem *number_item = PutTextInTable (data->_canvas_table,
                                                     text,
                                                     0,
                                                     0);
        SetTableItemAttribute (number_item, "y-align", 0.5);
        g_object_set (number_item,
                      "fill-color", "blue",
                      "font", "Bold",
                      NULL);
        g_free (text);
      }
    }

    if (   (winner == NULL)
        && data->_match->GetPlayerA ()
        && data->_match->GetPlayerB ())
    {
      data->_print_item = table->PutStockIconInTable (data->_canvas_table,
                                                      GTK_STOCK_PRINT,
                                                      0,
                                                      1);
      SetTableItemAttribute (data->_print_item, "y-align", 0.5);
    }

    {
      GString *string = g_string_new ("");

      // g_string_append_printf (string,
                              // "<<%d, %d>>", data->_row, data->_level);

      if (winner)
      {
        GSList *selected_attr = NULL;

        if (table->_filter)
        {
          selected_attr = table->_filter->GetSelectedAttrList ();
        }

        for (guint a = 0; a < g_slist_length (selected_attr); a++)
        {
          AttributeDesc *attr_desc;
          Attribute_c   *attr;

          attr_desc = (AttributeDesc *) g_slist_nth_data (selected_attr,
                                                          a);
          attr = winner->GetAttribute (attr_desc->_xml_name);

          if (attr)
          {
            if (a > 0)
            {
              g_string_append (string,
                               "  ");
            }
            g_string_append (string,
                             attr->GetUserImage ());
          }
        }
      }

      data->_player_item = PutTextInTable (data->_canvas_table,
                                           string->str,
                                           0,
                                           2);
      SetTableItemAttribute (data->_player_item, "y-align", 0.5);
      SetTableItemAttribute (data->_player_item, "x-expand", 1U);
      SetTableItemAttribute (data->_player_item, "x-fill", 1U);

      g_string_free (string,
                     TRUE);
    }

    if (parent)
    {
      if (winner)
      {
        GooCanvasItem *goo_rect;
        GooCanvasItem *score_text;
        NodeData      *parent_data = (NodeData *) parent->data;
        guint          position    = g_node_child_position (parent, node);

        table->SetPlayer (parent_data->_match,
                          winner,
                          position);

        // Rectangle
        {
          goo_rect = goo_canvas_rect_new (data->_canvas_table,
                                          0, 0,
                                          _score_rect_size, _score_rect_size,
                                          "line-width", 0.0,
                                          "pointer-events", GOO_CANVAS_EVENTS_VISIBLE,
                                          NULL);

          PutInTable (data->_canvas_table,
                      goo_rect,
                      0,
                      3);
          SetTableItemAttribute (goo_rect, "y-align", 0.5);
        }

        // Score Text
        {
          Score_c *score       = parent_data->_match->GetScore (winner);
          gchar   *score_image = score->GetImage ();

          score_text = goo_canvas_text_new (data->_canvas_table,
                                            score_image,
                                            0, 0,
                                            -1,
                                            GTK_ANCHOR_CENTER,
                                            "font", "Sans bold 18px",
                                            NULL);
          g_free (score_image);

          PutInTable (data->_canvas_table,
                      score_text,
                      0,
                      3);
          SetTableItemAttribute (score_text, "x-align", 0.5);
          SetTableItemAttribute (score_text, "y-align", 0.5);
        }

        if (   parent_data->_match->GetPlayerA ()
            && parent_data->_match->GetPlayerB ())
        {
          table->_score_collector->AddCollectingPoint (goo_rect,
                                                       score_text,
                                                       parent_data->_match,
                                                       winner);
          table->_score_collector->AddCollectingTrigger (data->_player_item);

          if (position == 0)
          {
            parent_data->_match->SetData (table, "player_A_rect", goo_rect);
          }
          else
          {
            table->_score_collector->SetNextCollectingPoint (goo_rect,
                                                             (GooCanvasItem *) parent_data->_match->GetData (table, "player_A_rect"));
            table->_score_collector->SetNextCollectingPoint ((GooCanvasItem *) parent_data->_match->GetData (table, "player_A_rect"),
                                                             goo_rect);
          }
        }
      }
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean Table::DeleteNode (GNode *node,
                            Table *table)
{
  NodeData *data = (NodeData *) node->data;

  Object_c::TryToRelease (data->_match);

  return FALSE;
}

// --------------------------------------------------------------------------------
void Table::AddFork (GNode *to)
{
  NodeData *to_data = NULL;
  NodeData *data = new NodeData;
  GNode    *node;

  if (to)
  {
    to_data = (NodeData *) to->data;
  }

  if (to == NULL)
  {
    data->_expected_winner_rank = 1;
    data->_level                = _nb_levels;
    data->_row                  = 1 << (_nb_levels - 1);
    node = g_node_new (data);
    _tree_root = node;
  }
  else
  {
    data->_level = to_data->_level - 1;

    if (g_node_n_children (to) == 0)
    {
      data->_row = to_data->_row - (1 << (data->_level - 1));
    }
    else
    {
      data->_row = to_data->_row + (1 << (data->_level - 1));
    }

    if (g_node_n_children (to) != (to_data->_expected_winner_rank % 2))
    {
      data->_expected_winner_rank = to_data->_expected_winner_rank;
    }
    else
    {
      data->_expected_winner_rank =    (1 << (_nb_levels - data->_level)) + 1
                                     - to_data->_expected_winner_rank;
    }
    node = g_node_append_data (to, data);
  }

  data->_canvas_table = NULL;
  data->_player_item  = NULL;
  data->_print_item   = NULL;
  data->_connector    = NULL;
  data->_match = new Match_c (_max_score);
  data->_match->SetData (this, "node", node);

  if ((to_data == NULL) || (data->_level > 1))
  {
    {
      GtkTreeIter  table_iter;
      gchar       *level_text = GetLevelImage (data->_level - 1);
      GtkTreePath *path = gtk_tree_path_new_from_indices (_nb_levels - data->_level,
                                                          -1);

      data->_match->SetData (this,
                             "level", level_text, g_free);

      if (gtk_tree_model_get_iter (GTK_TREE_MODEL (_quick_search_treestore),
                                   &table_iter,
                                   path) == FALSE)
      {
        gtk_tree_store_append (_quick_search_treestore,
                               &table_iter,
                               NULL);
        gtk_tree_store_set (_quick_search_treestore, &table_iter,
                            QUICK_MATCH_NAME_COLUMN, level_text,
                            QUICK_MATCH_VISIBILITY_COLUMN, 1,
                            -1);
      }

      gtk_tree_path_free (path);

      {
        GtkTreeIter  iter;
        gint        *indices;

        gtk_tree_store_append (_quick_search_treestore,
                               &iter,
                               &table_iter);

        path = gtk_tree_model_get_path (GTK_TREE_MODEL (_quick_search_treestore),
                                        &iter);
        indices = gtk_tree_path_get_indices (path);

        data->_match->SetData (this,
                               "quick_search_path", path, (GDestroyNotify) gtk_tree_path_free);
        data->_match->SetData (this,
                               "number", (void *) (indices[1]+1));
      }
    }

    AddFork (node);
    AddFork (node);
  }
  else
  {
    Player_c *player = (Player_c *) g_slist_nth_data (_attendees,
                                                      data->_expected_winner_rank - 1);

    if (player)
    {
      data->_match->SetPlayerA (player);
      data->_match->SetPlayerB (NULL);
    }
    else
    {
      data->_match->Release ();
      data->_match = NULL;
      to_data->_match->SetData (this,
                                "number", 0);
    }

    if (g_node_child_position (to, node) == 0)
    {
      to_data->_match->SetPlayerA (player);
      to_data->_match->SetPlayerB (NULL);
    }
    else
    {
      to_data->_match->SetPlayerA (NULL);
      to_data->_match->SetPlayerB (player);
    }
  }
}

// --------------------------------------------------------------------------------
gboolean Table::LoadNode (GNode *node,
                          Table *table)
{
  if (table->_xml_node && (table->_xml_node->type == XML_ELEMENT_NODE))
  {
    if (strcmp ((char *) table->_xml_node->name, "match") == 0)
    {
      gchar    *attr;
      Player_c *player;
      NodeData *data    = (NodeData *) node->data;

      if (data->_match)
      {
        attr = (gchar *) xmlGetProp (table->_xml_node, BAD_CAST "player_A");
        if (attr)
        {
          player = table->GetPlayerFromRef (atoi (attr));
          table->SetPlayer (data->_match,
                            player,
                            0);

          attr = (gchar *) xmlGetProp (table->_xml_node, BAD_CAST "score_A");
          if (attr)
          {
            data->_match->SetScore (player, atoi (attr));
          }
        }

        attr = (gchar *) xmlGetProp (table->_xml_node, BAD_CAST "player_B");
        if (attr)
        {
          player = table->GetPlayerFromRef (atoi (attr));
          table->SetPlayer (data->_match,
                            player,
                            1);

          attr = (gchar *) xmlGetProp (table->_xml_node, BAD_CAST "score_B");
          if (attr)
          {
            data->_match->SetScore (player, atoi (attr));
          }
        }

        if (table->_xml_node) table->_xml_node = table->_xml_node->next;
        if (table->_xml_node) table->_xml_node = table->_xml_node->next;
      }
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean Table::SaveNode (GNode *node,
                          Table *table)
{
  NodeData *data = (NodeData *) node->data;

  if (data->_match)
  {
    data->_match->Save (table->_xml_writer);
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
void Table::Wipe ()
{
  {
    Object_c::TryToRelease (_score_collector);
    _score_collector = new ScoreCollector (this,
                                           (ScoreCollector::OnNewScore_cbk) &Table::OnNewScore);

    _score_collector->SetConsistentColors ("LightGrey",
                                           "SkyBlue");
  }

  g_node_traverse (_tree_root,
                   G_POST_ORDER,
                   G_TRAVERSE_ALL,
                   -1,
                   (GNodeTraverseFunc) DeleteCanvasTable,
                   this);

  CanvasModule_c::Wipe ();
  _main_table = NULL;
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_table_print_toolbutton_clicked (GtkWidget *widget,
                                                                   Object_c  *owner)
{
  Table *t = dynamic_cast <Table *> (owner);

  t->Print ();
}

// --------------------------------------------------------------------------------
void Table::OnPlugged ()
{
  CanvasModule_c::OnPlugged ();

  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("table_classification_toggletoolbutton")),
                                     FALSE);

  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("input_toolbutton")),
                                     FALSE);
}

// --------------------------------------------------------------------------------
void Table::OnUnPlugged ()
{
  DeleteTree ();

  Object_c::TryToRelease (_score_collector);
  _score_collector = NULL;

  Object_c::TryToRelease (_quick_score_collector);
  _quick_score_collector = NULL;

  gtk_list_store_clear (_from_table_liststore);
  gtk_tree_store_clear (_quick_search_treestore);
  gtk_tree_store_clear (GTK_TREE_STORE (_quick_search_filter));

  CanvasModule_c::OnPlugged ();
}

// --------------------------------------------------------------------------------
void Table::OnLocked ()
{
  DisableSensitiveWidgets ();

  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("input_toolbutton")),
                                     FALSE);

  if (_score_collector)
  {
    _score_collector->Lock ();
  }
}

// --------------------------------------------------------------------------------
void Table::OnUnLocked ()
{
  EnableSensitiveWidgets ();

  if (_score_collector)
  {
    _score_collector->UnLock ();
  }
}

// --------------------------------------------------------------------------------
gboolean Table::AddToClassification (GNode *node,
                                     Table *table)
{
  NodeData *data = (NodeData *) node->data;

  if (data->_match)
  {
    Player_c *winner = data->_match->GetWinner ();

    if (winner && g_slist_find (table->_result_list,
                                winner) == NULL)
    {
      table->_result_list = g_slist_append (table->_result_list,
                                            winner);
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
void Table::OnAttrListUpdated ()
{
  Display ();
}

// --------------------------------------------------------------------------------
void Table::FillInConfig ()
{
  gchar *text = g_strdup_printf ("%d", _max_score);

  gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("max_score_entry")),
                      text);
  g_free (text);

  gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("name_entry")),
                      GetName ());
}

// --------------------------------------------------------------------------------
void Table::ApplyConfig ()
{
  {
    GtkWidget *name_w = _glade->GetWidget ("name_entry");
    gchar     *name   = (gchar *) gtk_entry_get_text (GTK_ENTRY (name_w));

    SetName (name);
  }

  {
    GtkWidget *max_score_w = _glade->GetWidget ("max_score_entry");

    if (max_score_w)
    {
      gchar *str = (gchar *) gtk_entry_get_text (GTK_ENTRY (max_score_w));

      if (str)
      {
        _max_score = atoi (str);
      }
    }
  }
}

// --------------------------------------------------------------------------------
void Table::OnFromTableComboboxChanged ()
{
  _nb_level_to_display = _nb_levels - gtk_combo_box_get_active (GTK_COMBO_BOX (_glade->GetWidget ("from_table_combobox")));

  Display ();
}

// --------------------------------------------------------------------------------
gboolean Table::Stuff (GNode *node,
                       Table *table)
{
  NodeData *data = (NodeData *) node->data;

  if (   (data->_level == table->_level_filter)
      && (data->_match))
  {
    Player_c *A = data->_match->GetPlayerA ();
    Player_c *B = data->_match->GetPlayerB ();

    if (A && B)
    {
      Player_c *winner;

      if (g_random_boolean ())
      {
        data->_match->SetScore (A, table->_max_score);
        data->_match->SetScore (B, g_random_int_range (0,
                                                       table->_max_score));
        winner = A;
      }
      else
      {
        data->_match->SetScore (A, g_random_int_range (0,
                                                       table->_max_score));
        data->_match->SetScore (B, table->_max_score);
        winner = B;
      }

      {
        GNode *parent = node->parent;

        if (parent)
        {
          NodeData *parent_data = (NodeData *) parent->data;

          table->SetPlayer (parent_data->_match,
                            winner,
                            g_node_child_position (parent, node));
        }
      }
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
void Table::SetPlayer (Match_c  *to_match,
                       Player_c *player,
                       guint     position)
{
  if (position == 0)
  {
    to_match->SetPlayerA (player);
  }
  else
  {
    to_match->SetPlayerB (player);
  }

  {
    GtkTreePath *path;
    GtkTreeIter  iter;

    path = (GtkTreePath *) to_match->GetData (this,
                                              "quick_search_path");

    if (   path
        && gtk_tree_model_get_iter (GTK_TREE_MODEL (_quick_search_treestore),
                                    &iter,
                                    path))
    {
      Player_c *A      = to_match->GetPlayerA ();
      Player_c *B      = to_match->GetPlayerB ();
      gchar    *A_name = NULL;
      gchar    *B_name = NULL;

      if (A)
      {
        Attribute_c *attr = A->GetAttribute ("name");

        if (attr)
        {
          A_name = attr->GetUserImage ();
        }
      }

      if (B)
      {
        Attribute_c *attr = B->GetAttribute ("name");

        if (attr)
        {
          B_name = attr->GetUserImage ();
        }
      }

      if (A_name && B_name)
      {
        gchar *name_string = g_strdup_printf ("%s / %s", A_name, B_name);
        guint  number      = (guint) to_match->GetData (this, "number");
        gchar *path_string;

        path_string = g_strdup_printf ("%s - No %d", (gchar *) to_match->GetData (this, "level"), number);
        gtk_tree_store_set (_quick_search_treestore, &iter,
                            QUICK_MATCH_PATH_COLUMN, path_string,
                            QUICK_MATCH_NAME_COLUMN, name_string,
                            QUICK_MATCH_COLUMN, to_match,
                            QUICK_MATCH_VISIBILITY_COLUMN, 1,
                            -1);
        g_free (path_string);
        g_free (name_string);
      }

      g_free (A_name);
      g_free (B_name);
    }
  }
}

// --------------------------------------------------------------------------------
void Table::OnStuffClicked ()
{
  for (_level_filter = 1; _level_filter <= _nb_levels; _level_filter++)
  {
    g_node_traverse (_tree_root,
                     G_POST_ORDER,
                     G_TRAVERSE_ALL,
                     _nb_levels - _level_filter + 1,
                     (GNodeTraverseFunc) Stuff,
                     this);
  }

  OnAttrListUpdated ();
}

// --------------------------------------------------------------------------------
void Table::OnInputToggled (GtkWidget *widget)
{
  GtkWidget *input_box = _glade->GetWidget ("input_hbox");

  if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (widget)))
  {
    gtk_widget_show (input_box);
  }
  else
  {
    gtk_widget_hide (input_box);
  }
}

// --------------------------------------------------------------------------------
GSList *Table::GetCurrentClassification ()
{
  _result_list = NULL;

  g_node_traverse (_tree_root,
                   G_LEVEL_ORDER,
                   G_TRAVERSE_ALL,
                   -1,
                   (GNodeTraverseFunc) AddToClassification,
                   this);

  return _result_list;
}

// --------------------------------------------------------------------------------
void Table::OnSearchMatch ()
{
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (_glade->GetWidget ("quick_search_combobox")),
                                     &iter))
  {
    Match_c *match;

    gtk_tree_model_get (GTK_TREE_MODEL (_quick_search_filter),
                        &iter,
                        QUICK_MATCH_COLUMN, &match,
                        -1);

    if (match)
    {
      Player_c *playerA = match->GetPlayerA ();
      Player_c *playerB = match->GetPlayerB ();

      if (playerA && playerB)
      {
        Attribute_c *attr;

        _quick_score_collector->SetMatch (_quick_score_A,
                                          match,
                                          playerA);
        attr = playerA->GetAttribute ("name");
        gtk_widget_show (_glade->GetWidget ("fencerA_label"));
        gtk_label_set_text (GTK_LABEL (_glade->GetWidget ("fencerA_label")),
                            attr->GetUserImage ());


        _quick_score_collector->SetMatch (_quick_score_B,
                                          match,
                                          playerB);
        attr = playerB->GetAttribute ("name");
        gtk_widget_show (_glade->GetWidget ("fencerB_label"));
        gtk_label_set_text (GTK_LABEL (_glade->GetWidget ("fencerB_label")),
                            attr->GetUserImage ());
        return;
      }
    }
  }

  gtk_widget_hide (_glade->GetWidget ("fencerA_label"));
  gtk_widget_hide (_glade->GetWidget ("fencerB_label"));

  _quick_score_collector->Wipe (_quick_score_A);
  _quick_score_collector->Wipe (_quick_score_B);
}

// --------------------------------------------------------------------------------
gchar *Table::GetLevelImage (guint level)
{
  gchar *image;

  if (level == _nb_levels)
  {
    image = g_strdup_printf ("Vainqueur");
  }
  else if (level == _nb_levels - 1)
  {
    image = g_strdup_printf ("Finale");
  }
  else if (level == _nb_levels - 2)
  {
    image = g_strdup_printf ("Demi-finale");
  }
  else
  {
    image = g_strdup_printf ("Tableau de %d", 1 << (_nb_levels - level));
  }

  return image;
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_table_filter_toolbutton_clicked (GtkWidget *widget,
                                                                    Object_c  *owner)
{
  Table *t = dynamic_cast <Table *> (owner);

  t->SelectAttributes ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_from_table_combobox_changed (GtkWidget *widget,
                                                                Object_c  *owner)
{
  Table *t = dynamic_cast <Table *> (owner);

  t->OnFromTableComboboxChanged ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_table_classification_toggletoolbutton_toggled (GtkWidget *widget,
                                                                                  Object_c  *owner)
{
  Table *t = dynamic_cast <Table *> (owner);

  t->ToggleClassification (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (widget)));
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_table_stuff_toolbutton_clicked (GtkWidget *widget,
                                                                   Object_c  *owner)
{
  Table *t = dynamic_cast <Table *> (owner);

  t->OnStuffClicked ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_input_toolbutton_toggled (GtkWidget *widget,
                                                             Object_c  *owner)
{
  Table *t = dynamic_cast <Table *> (owner);

  t->OnInputToggled (widget);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_quick_search_combobox_changed (GtkWidget *widget,
                                                                  Object_c  *owner)
{
  Table *t = dynamic_cast <Table *> (owner);

  t->OnSearchMatch ();
}
