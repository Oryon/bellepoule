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

const gchar *Table::_class_name     = "table";
const gchar *Table::_xml_class_name = "table_stage";

// --------------------------------------------------------------------------------
Table::Table (StageClass *stage_class)
: Stage_c (stage_class),
  CanvasModule_c ("table.glade")
{
  _main_table = NULL;
  _tree_root  = NULL;

  _max_score = 5;

  {
    ShowAttribute ("name");
    ShowAttribute ("club");
  }
}

// --------------------------------------------------------------------------------
Table::~Table ()
{
  Wipe ();
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
void Table::OnPlugged ()
{
  CanvasModule_c::OnPlugged ();

  Display ();
}

// --------------------------------------------------------------------------------
void Table::Display ()
{
  if (_main_table)
  {
    goo_canvas_item_remove (_main_table);
  }

  {
    GooCanvasItem *root = GetRootItem ();
    guint          nb_players;

    {
      Stage_c *previous_stage = GetPreviousStage ();

      _attendees  = previous_stage->GetResult ();
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
    }
    _nb_levels++;

    if (root)
    {
      _main_table = goo_canvas_table_new (root,
                                          "row-spacing",    10.0,
                                          "column-spacing", 40.0,
                                          NULL);

      // Header
      for (gint l = _nb_levels; l > 0; l--)
      {
        GooCanvasItem *level_header;
        GooCanvasItem *text_item;

        level_header = goo_canvas_table_new (_main_table,
                                             NULL);

        {
          gchar *text = g_strdup_printf ("Tableau de %d", 1 << (_nb_levels - l));

          text_item = PutInTable (level_header,
                                  1,
                                  0,
                                  text);
          g_object_set (G_OBJECT (text_item),
                        "font",       "Sans bold 18px",
                        "fill_color", "medium blue",
                        NULL);
          g_free (text);
        }

        PutInTable (_main_table,
                    level_header,
                    0,
                    l);
        SetTableItemAttribute (level_header, "x-align", 0.5);
      }

      // Recursively build the node tree
      AddFork (NULL);

      // Fill in first level
      g_node_traverse (_tree_root,
                       G_IN_ORDER,
                       G_TRAVERSE_LEAVES,
                       -1,
                       (GNodeTraverseFunc) FillInNode,
                       this);

      DrawAllConnectors ();
    }
  }
}

// --------------------------------------------------------------------------------
void Table::DrawAllConnectors ()
{
  g_node_traverse (_tree_root,
                   G_IN_ORDER,
                   G_TRAVERSE_ALL,
                   -1,
                   (GNodeTraverseFunc) DrawConnectors,
                   this);
}

// --------------------------------------------------------------------------------
gboolean Table::DrawConnectors (GNode *node,
                                Table *table)
{
  GNode *parent = node->parent;
  if (parent)
  {
    NodeData        *parent_data = (NodeData *) parent->data;
    GooCanvasBounds  parent_bounds;
    NodeData        *data = (NodeData *) node->data;
    GooCanvasBounds  bounds;

    goo_canvas_item_get_bounds (parent_data->_canvas_table,
                                &parent_bounds);

    goo_canvas_item_get_bounds (data->_canvas_table,
                                &bounds);

    //if (g_node_prev_sibling (node) == NULL)
    {
      goo_canvas_polyline_new (table->GetRootItem (),
                               FALSE,
                               3,
                               bounds.x1 - 40.0/2, bounds.y2,
                               parent_bounds.x1 - 40.0/2, bounds.y2,
                               parent_bounds.x1 - 40.0/2, parent_bounds.y2,
                               "line-width", 3.0,
                               NULL);
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean Table::FillInNode (GNode *node,
                            Table *table)
{
  NodeData *data   = (NodeData *) node->data;
  Player_c *player = (Player_c *) g_slist_nth_data (table->_attendees,
                                                    data->_expected_winner_rank - 1);

  if (player)
  {
    GooCanvasItem *player_item;
    GString       *string = g_string_new ("");

    data->_match = new Match_c (player,
                                NULL,
                                table->_max_score);

    for (guint a = 0; a < g_slist_length (table->_attr_list); a++)
    {
      gchar       *attr_name;
      Attribute_c *attr;

      attr_name = (gchar *) g_slist_nth_data (table->_attr_list,
                                              a);
      attr = player->GetAttribute (attr_name);

      if (attr)
      {
        string = g_string_append (string,
                                  attr->GetStringImage ());
        string = g_string_append (string,
                                  "  ");
      }
    }

    player_item = PutInTable (data->_canvas_table,
                              0,
                              1,
                              string->str);
    SetTableItemAttribute (player_item, "y-align", 0.5);

    g_string_free (string,
                   TRUE);
  }

#if 0
  {
    GNode *previous = g_node_prev_sibling (node);

    if (previous)
    {
      NodeData *previous_data = (NodeData *) previous->data;
      GNode    *parent        = node->parent;
      NodeData *parent_data   = (NodeData *) parent->data;

      parent_data->_match = new Match_c (previous_data->_match->GetWinner (),
                                         data->_match->GetWinner (),
                                         table->_max_score);
    }
  }
#endif

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

  data->_match = NULL;

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

  if ((to_data == NULL) || (data->_level > 1))
  {
    AddFork (node);
    AddFork (node);
  }

  {
    GooCanvasItem *print_item;
    GooCanvasItem *entry_item;
    GtkWidget     *image;
    GdkPixbuf     *pixbuf;

    data->_canvas_table = goo_canvas_table_new (_main_table,
                                                "column-spacing", 40.0,
                                                NULL);

    image  = gtk_image_new ();
    g_object_ref_sink (image);
    pixbuf = gtk_widget_render_icon (image,
                                     GTK_STOCK_PRINT,
                                     GTK_ICON_SIZE_BUTTON,
                                     NULL);

    print_item = goo_canvas_image_new (data->_canvas_table,
                                       pixbuf,
                                       0.0,
                                       0.0,
                                       NULL);

    {
      GtkWidget *gtk_entry  = gtk_entry_new ();

      gchar *toto = g_strdup_printf ("%d", data->_expected_winner_rank);
      gtk_entry_set_text (GTK_ENTRY (gtk_entry),
                          toto);
      entry_item = goo_canvas_widget_new (data->_canvas_table,
                                          gtk_entry,
                                          0,
                                          0,
                                          30,
                                          30,
                                          NULL);
      g_free (toto);
    }

    PutInTable (data->_canvas_table,
                print_item,
                0,
                0);
    SetTableItemAttribute (print_item, "y-align", 0.5);

    PutInTable (data->_canvas_table,
                entry_item,
                0,
                2);
    SetTableItemAttribute (entry_item, "y-align", 0.5);

    PutInTable (_main_table,
                data->_canvas_table,
                data->_row,
                data->_level);

    g_object_unref (image);
  }
}

// --------------------------------------------------------------------------------
Stage_c *Table::CreateInstance (StageClass *stage_class)
{
  return new Table (stage_class);
}

// --------------------------------------------------------------------------------
void Table::Load (xmlNode *xml_node)
{
}

// --------------------------------------------------------------------------------
void Table::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST _xml_class_name);
  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "name",
                                     "%s", GetName ());
  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void Table::Wipe ()
{
  CanvasModule_c::Wipe ();

  g_node_destroy (_tree_root);
  _tree_root = NULL;

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
void Table::Enter ()
{
  EnableSensitiveWidgets ();

  Display ();
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
  Wipe ();
  Display ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_table_filter_toolbutton_clicked (GtkWidget *widget,
                                                                    Object_c  *owner)
{
  Table *t = dynamic_cast <Table *> (owner);

  t->SelectAttributes ();
}
