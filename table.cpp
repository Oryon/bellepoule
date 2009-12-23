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

#include "attribute.hpp"
#include "player.hpp"
#include "table.hpp"

const gchar   *Table::_class_name     = "table";
const gchar   *Table::_xml_class_name = "table_stage";
const gdouble  Table::_level_spacing  = 10.0;

typedef enum
{
  NAME_COLUMN,
  STATUS_COLUMN
} ColumnId;

extern "C" G_MODULE_EXPORT void on_from_table_combobox_changed (GtkWidget *widget,
                                                                Object_c  *owner);

// --------------------------------------------------------------------------------
Table::Table (StageClass *stage_class)
: Stage_c (stage_class),
  CanvasModule_c ("table.glade")
{
  _main_table = NULL;
  _tree_root  = NULL;

  _score_collector = NULL;

  _max_score = 10;

  {
    ShowAttribute ("rank");
    ShowAttribute ("name");
    ShowAttribute ("club");
  }

  _from_table_liststore = GTK_LIST_STORE (_glade->GetObject ("table_liststore"));
}

// --------------------------------------------------------------------------------
Table::~Table ()
{
  DeleteTree ();
  Object_c::Release (_score_collector);

  gtk_list_store_clear (_from_table_liststore);
  g_object_unref (_from_table_liststore);
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
Stage_c *Table::CreateInstance (StageClass *stage_class)
{
  return new Table (stage_class);
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
    for (gint l = _nb_level_to_display; l > 0; l--)
    {
      GooCanvasItem *level_header;

      level_header = goo_canvas_table_new (_main_table,
                                           NULL);

      PutStockIconInTable (level_header,
                           GTK_STOCK_APPLY,
                           0, 0);

      {
        GooCanvasItem *text_item;
        gchar         *text = g_strdup_printf ("Tableau de %d", 1 << (_nb_level_to_display - l));

        text_item = PutTextInTable (level_header,
                                    text,
                                    0,
                                    1);
        g_object_set (G_OBJECT (text_item),
                      "font", "Sans bold 18px",
                      NULL);
        g_free (text);
      }

      PutInTable (_main_table,
                  level_header,
                  0,
                  l);
      SetTableItemAttribute (level_header, "x-align", 0.5);
    }

    g_node_traverse (_tree_root,
                     G_POST_ORDER,
                     G_TRAVERSE_ALL,
                     _nb_level_to_display,
                     (GNodeTraverseFunc) FillInNode,
                     this);

    DrawAllConnectors ();
  }
}

// --------------------------------------------------------------------------------
void Table::Garnish ()
{
  DeleteTree ();
  CreateTree ();
}

// --------------------------------------------------------------------------------
void Table::Load (xmlNode *xml_node)
{
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
void Table::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST _xml_class_name);
  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "name",
                                     "%s", GetName ());

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
void Table::OnNewScore (CanvasModule_c *client,
                        Match_c        *match,
                        Player_c       *player)
{
  Table *table = dynamic_cast <Table *> (client);
  GNode *node  = (GNode *) match->GetData ("Table::node");

  FillInNode (node,
              table);
  FillInNode (node->parent,
              table);

  table->DrawAllConnectors ();
}

// --------------------------------------------------------------------------------
void Table::DeleteTree ()
{
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

  DeleteTree ();
  AddFork (NULL);

  {
    g_signal_handlers_disconnect_by_func (_glade->GetWidget ("from_table_combobox"),
                                          (void *) on_from_table_combobox_changed,
                                          (Object_c *) this);
    gtk_list_store_clear (_from_table_liststore);

    for (guint i = 1; i <= _nb_levels; i++)
    {
      GtkTreeIter   iter;
      gchar        *text;

      gtk_list_store_append (_from_table_liststore,
                             &iter);

      text = g_strdup_printf ("Tableau de %d", 1 << (_nb_levels - i));
      gtk_list_store_set (_from_table_liststore, &iter,
                          NAME_COLUMN, text,
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

  table->WipeItem (data->_player_item);
  data->_player_item = NULL;

  table->WipeItem (data->_print_item);
  data->_print_item = NULL;

  table->WipeItem (data->_connector);
  data->_connector = NULL;

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean Table::DeleteCanvasTable (GNode *node,
                                   Table *table)
{
  NodeData *data = (NodeData *) node->data;

  WipeNode (node,
            table);

  table->WipeItem (data->_canvas_table);
  data->_canvas_table = NULL;

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean Table::DrawConnector (GNode *node,
                               Table *table)
{
  NodeData        *data   = (NodeData *) node->data;
  Player_c        *winner = data->_match->GetWinner ();
  GNode           *parent = node->parent;
  GooCanvasBounds  bounds;

  table->WipeItem (data->_connector);
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

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean Table::FillInNode (GNode *node,
                            Table *table)
{
  GNode    *parent = node->parent;
  NodeData *data   = (NodeData *) node->data;
  Player_c *winner = data->_match->GetWinner ();

  WipeNode (node,
            table);

  if (data->_canvas_table == NULL)
  {
    data->_canvas_table = goo_canvas_table_new (table->_main_table,
                                                "column-spacing", table->_level_spacing,
                                                NULL);
    SetTableItemAttribute (data->_canvas_table, "x-expand", 1U);
    SetTableItemAttribute (data->_canvas_table, "x-fill", 1U);
    PutInTable (table->_main_table,
                data->_canvas_table,
                data->_row,
                data->_level - (table->_nb_levels - table->_nb_level_to_display));
  }

  {
    GString *string = g_string_new ("");

    if (winner)
    {
      for (guint a = 0; a < g_slist_length (table->_attr_list); a++)
      {
        gchar       *attr_name;
        Attribute_c *attr;

        attr_name = (gchar *) g_slist_nth_data (table->_attr_list,
                                                a);
        attr = winner->GetAttribute (attr_name);

        if (attr)
        {
          if (a > 0)
          {
            string = g_string_append (string,
                                      "  ");
          }
          string = g_string_append (string,
                                    attr->GetStringImage ());
        }
      }
    }

    data->_player_item = PutTextInTable (data->_canvas_table,
                                         string->str,
                                         0,
                                         1);
    SetTableItemAttribute (data->_player_item, "y-align", 0.5);
    SetTableItemAttribute (data->_player_item, "x-expand", 1U);
    SetTableItemAttribute (data->_player_item, "x-fill", 1U);

    g_string_free (string,
                   TRUE);
  }

  if (   (winner == NULL)
      && (G_NODE_IS_LEAF (node) == FALSE)
      && (data->_match->HasSinglePlayer () != TRUE))
  {
    // Print Icon
    data->_print_item = PutStockIconInTable (data->_canvas_table,
                                             GTK_STOCK_PRINT,
                                             0,
                                             0);
    SetTableItemAttribute (data->_print_item, "y-align", 0.5);
  }

  if (parent)
  {
    if (winner)
    {
      GooCanvasItem *goo_rect;
      GooCanvasItem *score_text;
      NodeData      *parent_data = (NodeData *) parent->data;
      guint          position    = g_node_child_position (parent, node);

      if (position == 0)
      {
        parent_data->_match->SetPlayerA (winner);
      }
      else
      {
        parent_data->_match->SetPlayerB (winner);
      }

      // Rectangle
      {
        goo_rect = goo_canvas_rect_new (data->_canvas_table,
                                        0, 0,
                                        30.0, 30.0,
                                        "line-width", 0.0,
                                        "pointer-events", GOO_CANVAS_EVENTS_VISIBLE,
                                        NULL);

        PutInTable (data->_canvas_table,
                    goo_rect,
                    0,
                    2);
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
                    2);
        SetTableItemAttribute (score_text, "x-align", 0.5);
        SetTableItemAttribute (score_text, "y-align", 0.5);
      }

      if (   (parent_data->_match->GetWinner () == NULL)
          && (data->_match->HasSinglePlayer ()  != FALSE))
      {
        table->_score_collector->AddCollectingPoint (goo_rect,
                                                     score_text,
                                                     parent_data->_match,
                                                     winner,
                                                     position);
        table->_score_collector->AddCollectingTrigger (data->_player_item);

        if (position == 0)
        {
          parent_data->_match->SetData ("Table::player_A_rect", goo_rect);
        }
        else
        {
          table->_score_collector->SetNextCollectingPoint (goo_rect,
                                                           (GooCanvasItem *) parent_data->_match->GetData ("Table::player_A_rect"));
          table->_score_collector->SetNextCollectingPoint ((GooCanvasItem *) parent_data->_match->GetData ("Table::player_A_rect"),
                                                           goo_rect);
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

  Object_c::Release (data->_match);

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
  data->_match->SetData ("Table::node", node);

  if ((to_data == NULL) || (data->_level > 1))
  {
    AddFork (node);
    AddFork (node);
  }
  else
  {
    Player_c *player = (Player_c *) g_slist_nth_data (_attendees,
                                                      data->_expected_winner_rank - 1);

    data->_match->SetPlayerA (player);
    data->_match->SetPlayerB (NULL);
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

      attr = (gchar *) xmlGetProp (table->_xml_node, BAD_CAST "player_A");
      if (attr)
      {
        player = table->GetPlayerFromRef (atoi (attr));
        data->_match->SetPlayerA (player);

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
        data->_match->SetPlayerB (player);

        attr = (gchar *) xmlGetProp (table->_xml_node, BAD_CAST "score_B");
        if (attr)
        {
          data->_match->SetScore (player, atoi (attr));
        }
      }
    }

    if (table->_xml_node) table->_xml_node = table->_xml_node->next;
    if (table->_xml_node) table->_xml_node = table->_xml_node->next;
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean Table::SaveNode (GNode *node,
                          Table *table)
{
  NodeData *data = (NodeData *) node->data;

  data->_match->Save (table->_xml_writer);

  return FALSE;
}

// --------------------------------------------------------------------------------
void Table::Wipe ()
{
  {
    Object_c::Release (_score_collector);
    _score_collector = new ScoreCollector (GetCanvas (),
                                           this,
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
void Table::OnLocked ()
{
  DisableSensitiveWidgets ();
}

// --------------------------------------------------------------------------------
void Table::OnUnLocked ()
{
  EnableSensitiveWidgets ();
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

        //pool->SetMaxScore (_max_score);
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
