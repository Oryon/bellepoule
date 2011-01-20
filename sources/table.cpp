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
#include "classification.hpp"
#include "table.hpp"

const gchar   *Table::_class_name     = N_("Table");
const gchar   *Table::_xml_class_name = "PhaseDeTableaux";
const gdouble  Table::_score_rect_size = 30.0;
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
                                                                Object    *owner);

// --------------------------------------------------------------------------------
Table::Table (StageClass *stage_class)
: Stage (stage_class),
  CanvasModule ("table.glade")
{
  _main_table     = NULL;
  _tree_root      = NULL;
  _level_status   = NULL;
  _match_to_print = NULL;
  _nb_levels      = 0;

  g_datalist_init (&_match_list);

  _score_collector = NULL;

  _max_score = new Data ("ScoreMax",
                         10);

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
#ifndef DEBUG
                               "ref",
#endif
                               "status",
                               "global_status",
                               "start_rank",
                               "final_rank",
                               "attending",
                               "exported",
                               "victories_ratio",
                               "indice",
                               "HS",
                               "rank",
                               NULL);
    filter = new Filter (attr_list,
                         this);

    filter->ShowAttribute ("previous_stage_rank");
    filter->ShowAttribute ("name");
    filter->ShowAttribute ("first_name");
    filter->ShowAttribute ("club");

    SetFilter (filter);
    filter->Release ();
  }

  {
    GSList *attr_list;
    Filter *filter;

    AttributeDesc::CreateList (&attr_list,
#ifndef DEBUG
                               "ref",
#endif
                               "global_status",
                               "start_rank",
                               "final_rank",
                               "attending",
                               "exported",
                               "victories_ratio",
                               "indice",
                               "HS",
                               NULL);
    filter = new Filter (attr_list,
                         this);

    filter->ShowAttribute ("rank");
    filter->ShowAttribute ("name");
    filter->ShowAttribute ("first_name");
    filter->ShowAttribute ("club");
    filter->ShowAttribute ("status");

    SetClassificationFilter (filter);
    filter->Release ();
  }

  {
    GtkWidget *content_area;

    _level_print_dialog = gtk_message_dialog_new_with_markup (NULL,
                                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                                              GTK_MESSAGE_QUESTION,
                                                              GTK_BUTTONS_OK_CANCEL,
                                                              gettext ("<b><big>Which score sheets of the selected table do you want to print?</big></b>"));

    content_area = gtk_dialog_get_content_area (GTK_DIALOG (_level_print_dialog));

    gtk_widget_reparent (_glade->GetWidget ("print_level_dialog_vbox"),
                         content_area);
  }

  {
    GtkWidget *content_area;

    _print_dialog = gtk_message_dialog_new_with_markup (NULL,
                                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                                        GTK_MESSAGE_QUESTION,
                                                        GTK_BUTTONS_OK_CANCEL,
                                                        gettext ("<b><big>What do you want to print?</big></b>"));

    gtk_window_set_title (GTK_WINDOW (_print_dialog),
                          gettext ("Table printing"));

    content_area = gtk_dialog_get_content_area (GTK_DIALOG (_print_dialog));

    gtk_widget_reparent (_glade->GetWidget ("print_table_dialog-vbox"),
                         content_area);
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
  DeleteTree ();

  Object::TryToRelease (_quick_score_collector);
  Object::TryToRelease (_score_collector);

  _max_score->Release ();

  if (_match_to_print)
  {
    g_slist_free (_match_to_print);
  }

  gtk_widget_destroy (_print_dialog);
  gtk_widget_destroy (_level_print_dialog);

  if (_match_list)
  {
    g_datalist_clear (&_match_list);
  }
}

// --------------------------------------------------------------------------------
void Table::Init ()
{
  RegisterStageClass (gettext (_class_name),
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
GooCanvasItem *Table::GetQuickScore (const gchar *container)
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
Stage *Table::CreateInstance (StageClass *stage_class)
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
        icon = g_strdup (GTK_STOCK_DIALOG_WARNING);
      }
      else if (_level_status[i]._is_over == TRUE)
      {
        icon = g_strdup (GTK_STOCK_APPLY);
      }
      else
      {
        icon = g_strdup (GTK_STOCK_EXECUTE);
      }

      _level_status[i]._status_item = Canvas::PutStockIconInTable (_level_status[i-nb_missing_level]._level_header,
                                                                   icon,
                                                                   0, 0);

      gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (_from_table_liststore),
                                     &iter,
                                     NULL,
                                     i-nb_missing_level);
      gtk_list_store_set (_from_table_liststore, &iter,
                          FROM_STATUS_COLUMN, icon,
                          -1);
      g_free (icon);
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
                                        "column-spacing", _level_spacing,
                                        NULL);

    // Header
    for (guint l = _nb_level_to_display; l > 0; l--)
    {
      guint level;

      _level_status[l-1]._level_header = goo_canvas_table_new (_main_table,
                                                               NULL);

      level = l + (_nb_levels - _nb_level_to_display);
      {
        GooCanvasItem *text_item;
        gchar         *text  = GetLevelImage (level);

        text_item = Canvas::PutTextInTable (_level_status[l-1]._level_header,
                                            text,
                                            0,
                                            1);
        g_object_set (G_OBJECT (text_item),
                      "font", "Sans bold 18px",
                      NULL);
        g_free (text);
      }

      if (level < _nb_levels)
      {
        GooCanvasItem *print_item;

        print_item = Canvas::PutStockIconInTable (_level_status[l-1]._level_header,
                                                  GTK_STOCK_PRINT,
                                                  1,
                                                  1);
        Canvas::SetTableItemAttribute (print_item, "x-align", 0.5);

        g_object_set_data (G_OBJECT (print_item), "level_to_print", (void *) level);
        g_signal_connect (print_item, "button-release-event",
                          G_CALLBACK (OnPrintLevel), this);
      }

      Canvas::PutInTable (_main_table,
                          _level_status[l-1]._level_header,
                          0,
                          l);

      Canvas::SetTableItemAttribute (_level_status[l-1]._level_header, "x-align", 0.5);
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
  Stage::LoadConfiguration (xml_node);

  if (_max_score)
  {
    _max_score->Load (xml_node);
  }
}

// --------------------------------------------------------------------------------
void Table::Load (xmlNode *xml_node)
{
  LoadConfiguration (xml_node);

  for (xmlNode *n = xml_node; n != NULL; n = n->next)
  {
    static gchar *level;

    if (n->type == XML_ELEMENT_NODE)
    {
      if (strcmp ((char *) xml_node->name, _xml_class_name) == 0)
      {
      }
      else if (strcmp ((char *) n->name, "Tireur") == 0)
      {
        if (_tree_root == NULL)
        {
          LoadAttendees (n);
        }
      }
      else if (strcmp ((char *) n->name, "SuiteDeTableaux") == 0)
      {
        if (_attendees)
        {
          CreateTree ();
        }
      }
      else if (strcmp ((char *) n->name, "Tableau") == 0)
      {
        level = (gchar *) xmlGetProp (n, BAD_CAST "Taille");
      }
      else if (strcmp ((char *) n->name, "Match") == 0)
      {
        gchar *attr   = (gchar *) xmlGetProp (n, BAD_CAST "ID");
        gchar *number = g_strdup_printf ("M%s.%s", level, attr);
        Match *match  = (Match *) g_datalist_get_data (&_match_list,
                                                       number);
        g_free (number);

        if (match)
        {
          LoadMatch (n,
                     match);
        }
      }
      else
      {
        return;
      }
    }
    Load (n->children);
  }
}

// --------------------------------------------------------------------------------
void Table::SaveConfiguration (xmlTextWriter *xml_writer)
{
  Stage::SaveConfiguration (xml_writer);

  if (_max_score)
  {
    _max_score->Save (xml_writer);
  }
}

// --------------------------------------------------------------------------------
void Table::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST _xml_class_name);

  SaveConfiguration (xml_writer);
  SaveAttendees (xml_writer);

  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST "SuiteDeTableaux");
  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "ID",
                                     "%s", "A");
  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "Titre",
                                     "%s", "");
  if (_nb_levels)
  {
    xmlTextWriterWriteFormatAttribute (xml_writer,
                                       BAD_CAST "NbDeTableaux",
                                       "%d", _nb_levels-1);
  }

  if (_tree_root)
  {
    for (guint i = 1; i < _nb_levels; i++)
    {
      guint size;

      size = 1 << (_nb_levels - i);

      xmlTextWriterStartElement (xml_writer,
                                 BAD_CAST "Tableau");
      xmlTextWriterWriteFormatAttribute (xml_writer,
                                         BAD_CAST "ID",
                                         "A%d", size);
      xmlTextWriterWriteFormatAttribute (xml_writer,
                                         BAD_CAST "Titre",
                                         "%s", GetLevelImage (i));
      xmlTextWriterWriteFormatAttribute (xml_writer,
                                         BAD_CAST "Taille",
                                         "%d", size);

      for (guint m = 0; m < size; m++)
      {
        gchar *number;
        Match *match;

        number = g_strdup_printf ("M%d.%d", size, m);
        match  = (Match *) g_datalist_get_data (&_match_list,
                                                number);
        if (match)
        {
          match->Save (xml_writer);
        }
        g_free (number);
      }

      xmlTextWriterEndElement (xml_writer);
    }
  }

  xmlTextWriterEndElement (xml_writer);
  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void Table::OnNewScore (ScoreCollector *score_collector,
                        CanvasModule   *client,
                        Match          *match,
                        Player         *player)
{
  Table *table = dynamic_cast <Table *> (client);
  GNode *node  = (GNode *) match->GetData (table, "node");

  FillInNode (node,
              table);

  if (score_collector == table->_quick_score_collector)
  {
    table->_score_collector->Refresh (match);
  }
  else
  {
    table->_quick_score_collector->Refresh (match);
  }

  if (node->parent)
  {
    FillInNode (node->parent,
                table);
  }

  table->RefreshLevelStatus ();
  table->DrawAllConnectors ();
}

// --------------------------------------------------------------------------------
void Table::DeleteTree ()
{
  g_signal_handlers_disconnect_by_func (_glade->GetWidget ("from_table_combobox"),
                                        (void *) on_from_table_combobox_changed,
                                        (Object *) this);

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
  guint nb_players = g_slist_length (_attendees->GetShortList ());

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
                                          (Object *) this);
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
                      (Object *) this);
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
    Player *winner = data->_match->GetWinner ();

    if (winner == NULL)
    {
      Player *A       = data->_match->GetPlayerA ();
      Player *B       = data->_match->GetPlayerB ();
      Score  *score_A = data->_match->GetScore (A);
      Score  *score_B = data->_match->GetScore (B);

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
    Player          *winner = data->_match->GetWinner ();
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
    Player *winner = data->_match->GetWinner ();

    WipeNode (node,
              table);

    if (data->_canvas_table == NULL)
    {
      guint row;

      data->_canvas_table = goo_canvas_table_new (table->_main_table,
                                                  "column-spacing", table->_level_spacing,
                                                  NULL);
      //Canvas::SetTableItemAttribute (data->_canvas_table, "x-expand", 1U);
      Canvas::SetTableItemAttribute (data->_canvas_table, "x-fill", 1U);

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
      Canvas::PutInTable (table->_main_table,
                          data->_canvas_table,
                          row,
                          data->_level - nb_missing_level);
    }

    {
      gchar *number = (gchar *) data->_match->GetData (table, "number");

      if (number)
      {
        GooCanvasItem *number_item = Canvas::PutTextInTable (data->_canvas_table,
                                                             number,
                                                             0,
                                                             0);
        Canvas::SetTableItemAttribute (number_item, "y-align", 0.5);
        g_object_set (number_item,
                      "fill-color", "grey",
                      "font", "Bold",
                      NULL);
      }
    }

    if (   (winner == NULL)
        && data->_match->GetPlayerA ()
        && data->_match->GetPlayerB ())
    {
      data->_print_item = Canvas::PutStockIconInTable (data->_canvas_table,
                                                       GTK_STOCK_PRINT,
                                                       1,
                                                       0);
      g_object_set_data (G_OBJECT (data->_print_item), "match_to_print", data->_match);
      g_signal_connect (data->_print_item, "button-release-event",
                        G_CALLBACK (OnPrintMatch), table);
    }

    if (winner)
    {
      GString *string = table->GetPlayerImage (winner);

      data->_player_item = Canvas::PutTextInTable (data->_canvas_table,
                                                   string->str,
                                                   0,
                                                   2);
      Canvas::SetTableItemAttribute (data->_player_item, "y-align", 0.5);

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

        if (   (parent_data->_match->GetWinner () == NULL)
            || (   parent_data->_match->GetPlayerA ()
                && parent_data->_match->GetPlayerB ()))
        {
          GtkWidget       *w    = gtk_combo_box_new_with_model (table->GetStatusModel ());
          GtkCellRenderer *cell = gtk_cell_renderer_pixbuf_new ();
          GooCanvasItem   *goo_item;

          gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (w), cell, FALSE);
          gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (w),
                                          cell, "pixbuf", AttributeDesc::DISCRETE_ICON,
                                          NULL);

          goo_item = goo_canvas_widget_new (data->_canvas_table,
                                            w,
                                            0.0,
                                            0.0,
                                            _score_rect_size*3.3/2.0,
                                            _score_rect_size,
                                            NULL);
          Canvas::PutInTable (data->_canvas_table,
                              goo_item,
                              0, 3);

          {
            Player::AttributeId  attr_id ("status", table->GetDataOwner ());
            Attribute           *attr = winner->GetAttribute (&attr_id);

            if (attr)
            {
              GtkTreeIter  iter;
              gboolean     iter_is_valid;
              gchar       *code;
              gchar       *text;

              if (parent_data->_match->IsDropped () == FALSE)
              {
                text = (gchar *) "Q";
              }
              else
              {
                text = (gchar *) attr->GetValue ();
              }

              iter_is_valid = gtk_tree_model_get_iter_first (table->GetStatusModel (),
                                                             &iter);
              for (guint i = 0; iter_is_valid; i++)
              {
                gtk_tree_model_get (table->GetStatusModel (),
                                    &iter,
                                    AttributeDesc::DISCRETE_XML_IMAGE, &code,
                                    -1);
                if (strcmp (text, code) == 0)
                {
                  gtk_combo_box_set_active_iter (GTK_COMBO_BOX (w),
                                                 &iter);

                  break;
                }
                iter_is_valid = gtk_tree_model_iter_next (table->GetStatusModel (),
                                                          &iter);
              }
            }
          }

          if (table->Locked ())
          {
            gtk_widget_set_sensitive (w,
                                      FALSE);
          }
          else
          {
            NodeData *parent_data;

            if (parent)
            {
              parent_data = (NodeData *) parent->data;

              g_object_set_data (G_OBJECT (w), "player_for_status", (void *) winner);
              g_object_set_data (G_OBJECT (w), "match_for_status", (void *) parent_data->_match);
              g_signal_connect (w, "changed",
                                G_CALLBACK (on_status_changed), table);
            }
          }
        }

        // Rectangle
        {
          goo_rect = goo_canvas_rect_new (data->_canvas_table,
                                          0, 0,
                                          _score_rect_size, _score_rect_size,
                                          "line-width", 0.0,
                                          "pointer-events", GOO_CANVAS_EVENTS_VISIBLE,
                                          NULL);

          Canvas::PutInTable (data->_canvas_table,
                              goo_rect,
                              0,
                              4);
          Canvas::SetTableItemAttribute (goo_rect, "y-align", 0.5);
        }

        // Score Text
        {
          Score *score       = parent_data->_match->GetScore (winner);
          gchar *score_image = score->GetImage ();

          score_text = goo_canvas_text_new (data->_canvas_table,
                                            score_image,
                                            0, 0,
                                            -1,
                                            GTK_ANCHOR_CENTER,
                                            "font", "Sans bold 18px",
                                            NULL);
          g_free (score_image);

          Canvas::PutInTable (data->_canvas_table,
                              score_text,
                              0,
                              4);
          Canvas::SetTableItemAttribute (score_text, "x-align", 0.5);
          Canvas::SetTableItemAttribute (score_text, "y-align", 0.5);
        }

        {
          Player *A = parent_data->_match->GetPlayerA ();
          Player *B = parent_data->_match->GetPlayerB ();

          if (   (parent_data->_match->GetWinner () == NULL)
              || ((A != NULL) && (B != NULL)))
          {
            parent_data->_match->SetData (winner, "collecting_point", goo_rect);

            table->_score_collector->AddCollectingPoint (goo_rect,
                                                         score_text,
                                                         parent_data->_match,
                                                         winner);
            table->_score_collector->AddCollectingTrigger (data->_player_item);

            if (A && B)
            {
              table->_score_collector->SetNextCollectingPoint ((GooCanvasItem *) parent_data->_match->GetData (A,
                                                                                                               "collecting_point"),
                                                               (GooCanvasItem *) parent_data->_match->GetData (B,
                                                                                                               "collecting_point"));
              table->_score_collector->SetNextCollectingPoint ((GooCanvasItem *) parent_data->_match->GetData (B,
                                                                                                               "collecting_point"),
                                                               (GooCanvasItem *) parent_data->_match->GetData (A,
                                                                                                               "collecting_point"));
            }
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

  Object::TryToRelease (data->_match);

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
  data->_match = new Match (_max_score);
  data->_match->SetData (this, "node", node);

  if ((to_data == NULL) || (data->_level > 1))
  {
    {
      GtkTreeIter  table_iter;
      GtkTreePath *path = gtk_tree_path_new_from_indices (_nb_levels - data->_level,
                                                          -1);

      data->_match->SetData (this,
                             "level", (void *) (data->_level-1));

      if (gtk_tree_model_get_iter (GTK_TREE_MODEL (_quick_search_treestore),
                                   &table_iter,
                                   path) == FALSE)
      {
        gtk_tree_store_append (_quick_search_treestore,
                               &table_iter,
                               NULL);
        gtk_tree_store_set (_quick_search_treestore, &table_iter,
                            QUICK_MATCH_NAME_COLUMN, GetLevelImage (data->_level - 1),
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

        {
          gchar *number = g_strdup_printf ("M%d.%d", 1 << (_nb_levels - data->_level + 1), indices[1]+1);

          data->_match->SetNumber (indices[1]+1);
          data->_match->SetData (this,
                                 "number",
                                 number,
                                 g_free);

          g_datalist_set_data (&_match_list,
                               number,
                               data->_match);
        }
      }
    }

    AddFork (node);
    AddFork (node);
  }
  else
  {
    Player *player = (Player *) g_slist_nth_data (_attendees->GetShortList (),
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

      g_datalist_remove_data (&_match_list,
                              (gchar *) to_data->_match->GetData (this, "number"));
      to_data->_match->RemoveData (this,
                                   "number");
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
void Table::LoadMatch (xmlNode *xml_node,
                       Match   *match)
{
  for (xmlNode *n = xml_node; n != NULL; n = n->next)
  {
    if (n->type == XML_ELEMENT_NODE)
    {
      static xmlNode *A = NULL;
      static xmlNode *B = NULL;

      if (strcmp ((char *) n->name, "Match") == 0)
      {
        gchar *attr = (gchar *) xmlGetProp (n, BAD_CAST "ID");

        A = NULL;
        B = NULL;
        if ((attr == NULL) || (atoi (attr) != match->GetNumber ()))
        {
          return;
        }
      }
      else if (strcmp ((char *) n->name, "Arbitre") == 0)
      {
      }
      else if (strcmp ((char *) n->name, "Tireur") == 0)
      {
        if (A == NULL)
        {
          A = n;
        }
        else
        {
          Player *dropped = NULL;

          B = n;

          LoadScore (A, match, 0, &dropped);
          LoadScore (B, match, 1, &dropped);

          if (dropped)
          {
            match->DropPlayer (dropped);
          }

          A = NULL;
          B = NULL;
          return;
        }
      }
      LoadMatch (n->children,
                 match);
    }
  }
}

// --------------------------------------------------------------------------------
void Table::LoadScore (xmlNode *xml_node,
                       Match   *match,
                       guint    player_index,
                       Player  **dropped)
{
  gboolean  is_the_best = FALSE;
  gchar    *attr        = (gchar *) xmlGetProp (xml_node, BAD_CAST "REF");
  Player   *player      = GetPlayerFromRef (atoi (attr));

  SetPlayer (match,
             player,
             player_index);

  attr = (gchar *) xmlGetProp (xml_node, BAD_CAST "Statut");
  if (attr && attr[0] == 'V')
  {
    is_the_best = TRUE;
  }

  attr = (gchar *) xmlGetProp (xml_node, BAD_CAST "Score");
  if (attr)
  {
    match->SetScore (player, atoi (attr), is_the_best);

    if (is_the_best == FALSE)
    {
      Player::AttributeId  attr_id ("status", GetDataOwner ());
      Attribute           *attr   = player->GetAttribute (&attr_id);
      gchar               *status = (gchar *) attr->GetValue ();

      if (status && ((*status == 'E') || (*status == 'A')))
      {
        *dropped = player;
      }
    }
  }
}

// --------------------------------------------------------------------------------
void Table::Wipe ()
{
  {
    Object::TryToRelease (_score_collector);
    _score_collector = new ScoreCollector (this,
                                           (ScoreCollector::OnNewScore_cbk) &Table::OnNewScore);

    _score_collector->SetConsistentColors ("LightGrey",
                                           "SkyBlue");
  }

  if (_tree_root)
  {
    g_node_traverse (_tree_root,
                     G_POST_ORDER,
                     G_TRAVERSE_ALL,
                     -1,
                     (GNodeTraverseFunc) DeleteCanvasTable,
                     this);
  }

  CanvasModule::Wipe ();
  _main_table = NULL;
}

// --------------------------------------------------------------------------------
void Table::LookForMatchToPrint (guint    level_to_print,
                                 gboolean all_sheet)
{
  if (_match_to_print)
  {
    g_slist_free (_match_to_print);
    _match_to_print = NULL;
  }

  {
    GtkTreeIter parent;
    gboolean    iter_is_valid;

    iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_quick_search_filter),
                                                   &parent);
    while (iter_is_valid)
    {
      GtkTreeIter iter;

      iter_is_valid = gtk_tree_model_iter_children (GTK_TREE_MODEL (_quick_search_filter),
                                                    &iter,
                                                    &parent);
      while (iter_is_valid)
      {
        Match *match;

        gtk_tree_model_get (GTK_TREE_MODEL (_quick_search_filter),
                            &iter,
                            QUICK_MATCH_COLUMN, &match,
                            -1);
        if (match)
        {
          guint level = (guint) match->GetData (this, "level");

          if ((level_to_print == 0) || (level_to_print == level))
          {
            gboolean already_printed = (gboolean) match->GetData (this, "printed");

            if (all_sheet || (already_printed != TRUE))
            {
              _match_to_print = g_slist_prepend (_match_to_print,
                                                 match);
            }
          }
        }
        iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_quick_search_filter),
                                                  &iter);
      }

      iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_quick_search_filter),
                                                &parent);
    }
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_table_print_toolbutton_clicked (GtkWidget *widget,
                                                                   Object    *owner)
{
  Table *t = dynamic_cast <Table *> (owner);

  t->OnPrint ();
}

// --------------------------------------------------------------------------------
void Table::OnPlugged ()
{
  CanvasModule::OnPlugged ();

  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("table_classification_toggletoolbutton")),
                                     FALSE);

  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("input_toolbutton")),
                                     FALSE);
}

// --------------------------------------------------------------------------------
void Table::OnUnPlugged ()
{
  DeleteTree ();

  Object::TryToRelease (_score_collector);
  _score_collector = NULL;

  gtk_list_store_clear (_from_table_liststore);
  gtk_tree_store_clear (_quick_search_treestore);
  gtk_tree_store_clear (GTK_TREE_STORE (_quick_search_filter));

  CanvasModule::OnPlugged ();
}

// --------------------------------------------------------------------------------
void Table::OnLocked (Reason reason)
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
    Player *winner = data->_match->GetWinner ();

    if (winner && g_slist_find (table->_result_list,
                                winner) == NULL)
    {
      table->_result_list = g_slist_append (table->_result_list,
                                            winner);
      winner->SetData (table, "level", (void *) data->_level);
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
  gchar *text = g_strdup_printf ("%d", _max_score->_value);

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
        _max_score->_value = atoi (str);
        if (_main_table)
        {
          OnAttrListUpdated ();
        }
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
    Player *A = data->_match->GetPlayerA ();
    Player *B = data->_match->GetPlayerB ();

    if (A && B)
    {
      Player *winner;

      if (g_random_boolean ())
      {
        data->_match->SetScore (A, table->_max_score->_value, TRUE);
        data->_match->SetScore (B, g_random_int_range (0,
                                                       table->_max_score->_value), FALSE);
        winner = A;
      }
      else
      {
        data->_match->SetScore (A, g_random_int_range (0,
                                                       table->_max_score->_value), FALSE);
        data->_match->SetScore (B, table->_max_score->_value, TRUE);
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
void Table::SetPlayer (Match  *to_match,
                       Player *player,
                       guint   position)
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
      Player *A      = to_match->GetPlayerA ();
      Player *B      = to_match->GetPlayerB ();
      gchar  *A_name = NULL;
      gchar  *B_name = NULL;

      if (A)
      {
        A_name = A->GetName ();
      }

      if (B)
      {
        B_name = B->GetName ();
      }

      if (A_name && B_name)
      {
        gchar *name_string = g_strdup_printf ("%s / %s", A_name, B_name);
        gchar *number      = (gchar *) to_match->GetData (this, "number");

        gtk_tree_store_set (_quick_search_treestore, &iter,
                            QUICK_MATCH_PATH_COLUMN, number,
                            QUICK_MATCH_NAME_COLUMN, name_string,
                            QUICK_MATCH_COLUMN, to_match,
                            QUICK_MATCH_VISIBILITY_COLUMN, 1,
                            -1);
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
void Table::OnMatchSheetToggled (GtkWidget *widget)
{
  GtkWidget *vbox = _glade->GetWidget ("match_sheet_vbox");

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
  {
    gtk_widget_set_sensitive (vbox, TRUE);
  }
  else
  {
    gtk_widget_set_sensitive (vbox, FALSE);
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

  _result_list = g_slist_sort_with_data (_result_list,
                                         (GCompareDataFunc) ComparePlayer,
                                         (void *) this);

  {
    Player::AttributeId *attr_id = new Player::AttributeId ("rank", this);
    GSList              *current;
    guint                previous_rank   = 0;
    Player              *previous_player = NULL;
    guint32              rand_seed = _rand_seed;

    _rand_seed = 0; // !!
    current = _result_list;
    for (guint i = 1; current; i++)
    {
      Player *player;

      player = (Player *) current->data;

      if (   previous_player
          && (   (ComparePlayer (player,
                                 previous_player,
                                 this) == 0)
              && (ComparePreviousRankPlayer (player,
                                             previous_player,
                                             0) == 0))
          || (i == 4))
      {
        player->SetAttributeValue (attr_id,
                                   previous_rank);
      }
      else
      {
        player->SetAttributeValue (attr_id,
                                   i);
        previous_rank = i;
      }

      previous_player = player;
      current = g_slist_next (current);
    }
    attr_id->Release ();
    _rand_seed = rand_seed; // !!
  }

  return _result_list;
}

// --------------------------------------------------------------------------------
gint Table::ComparePlayer (Player  *A,
                           Player  *B,
                           Table   *table)
{
  {
    Player::AttributeId attr_id ("status", table->GetDataOwner ());

    if (A->GetAttribute (&attr_id) && B->GetAttribute (&attr_id))
    {
      gchar *status_A = (gchar *) A->GetAttribute (&attr_id)->GetValue ();
      gchar *status_B = (gchar *) B->GetAttribute (&attr_id)->GetValue ();

      if ((status_A[0] == 'E') && (status_A[0] != status_B[0]))
      {
        return 1;
      }
      if ((status_B[0] == 'E') && (status_B[0] != status_A[0]))
      {
        return -1;
      }
    }
  }

  {
    gint level_A = (gint) A->GetData (table, "level");
    gint level_B = (gint) B->GetData (table, "level");

    if (level_A != level_B)
    {
      return level_B - level_A;
    }
  }

  return table->ComparePreviousRankPlayer (A,
                                           B,
                                           table->_rand_seed);
}

// --------------------------------------------------------------------------------
gint Table::ComparePreviousRankPlayer (Player  *A,
                                       Player  *B,
                                       guint32  rand_seed)
{
  Stage               *previous = GetPreviousStage ();
  Player::AttributeId  attr_id ("rank", previous);

  attr_id.MakeRandomReady (rand_seed);
  return Player::Compare (A,
                          B,
                          &attr_id);
}

// --------------------------------------------------------------------------------
void Table::OnSearchMatch ()
{
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (_glade->GetWidget ("quick_search_combobox")),
                                     &iter))
  {
    Match *match;

    gtk_tree_model_get (GTK_TREE_MODEL (_quick_search_filter),
                        &iter,
                        QUICK_MATCH_COLUMN, &match,
                        -1);

    if (match)
    {
      Player *playerA = match->GetPlayerA ();
      Player *playerB = match->GetPlayerB ();

      if (playerA && playerB)
      {
        gchar *name;

        name = playerA->GetName ();
        _quick_score_collector->SetMatch (_quick_score_A,
                                          match,
                                          playerA);
        gtk_widget_show (_glade->GetWidget ("fencerA_label"));
        gtk_label_set_text (GTK_LABEL (_glade->GetWidget ("fencerA_label")),
                            name);
        g_free (name);


        name = playerB->GetName ();
        _quick_score_collector->SetMatch (_quick_score_B,
                                          match,
                                          playerB);
        gtk_widget_show (_glade->GetWidget ("fencerB_label"));
        gtk_label_set_text (GTK_LABEL (_glade->GetWidget ("fencerB_label")),
                            name);
        g_free (name);
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
    image = g_strdup_printf (gettext ("Winner"));
  }
  else if (level == _nb_levels - 1)
  {
    image = g_strdup_printf (gettext ("Final"));
  }
  else if (level == _nb_levels - 2)
  {
    image = g_strdup_printf (gettext ("Semi-final"));
  }
  else
  {
    image = g_strdup_printf (gettext ("Table of %d"), 1 << (_nb_levels - level));
  }

  return image;
}

// --------------------------------------------------------------------------------
void Table::OnFilterClicked ()
{
  if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("table_classification_toggletoolbutton"))))
  {
    Classification *classification = GetClassification ();

    if (classification)
    {
      classification->SelectAttributes ();
    }
  }
  else
  {
    SelectAttributes ();
  }
}

// --------------------------------------------------------------------------------
void Table::OnBeginPrint (GtkPrintOperation *operation,
                          GtkPrintContext   *context)
{
  if (_print_full_table)
  {
    gdouble canvas_x;
    gdouble canvas_y;
    gdouble canvas_w;
    gdouble canvas_h;
    gdouble paper_w  = gtk_print_context_get_width (context);
    gdouble paper_h  = gtk_print_context_get_height (context);
    gdouble header_h = (PRINT_HEADER_HEIGHT+2) * paper_w  / 100;

    {
      GooCanvasBounds bounds;

      goo_canvas_item_get_bounds (GetRootItem (),
                                  &bounds);
      canvas_x = bounds.x1;
      canvas_y = bounds.y1;
      canvas_w = bounds.x2 - bounds.x1;
      canvas_h = bounds.y2 - bounds.y1;
    }

    {
      gdouble canvas_dpi;
      gdouble printer_dpi;

      g_object_get (G_OBJECT (GetCanvas ()),
                    "resolution-x", &canvas_dpi,
                    NULL);
      printer_dpi = gtk_print_context_get_dpi_x (context);

      _print_scale = printer_dpi/canvas_dpi;
    }

    _print_nb_x_pages = 1;
    _print_nb_y_pages = 1;
    if (   (canvas_w * _print_scale > paper_w)
           || (canvas_h * _print_scale > paper_h))
    {
      _print_nb_x_pages += (guint) (canvas_w * _print_scale / paper_w);
      _print_nb_y_pages += (guint) ((canvas_h * _print_scale + header_h) / paper_h);
    }

    gtk_print_operation_set_n_pages (operation,
                                     _print_nb_x_pages * _print_nb_y_pages);
  }
  else
  {
    guint nb_match;
    guint nb_page;

    nb_match = g_slist_length (_match_to_print);
    nb_page  = nb_match/4;

    if (nb_match%4 != 0)
    {
      nb_page++;
    }

    gtk_print_operation_set_n_pages (operation,
                                     nb_page);
  }
}

// --------------------------------------------------------------------------------
void Table::OnDrawPage (GtkPrintOperation *operation,
                        GtkPrintContext   *context,
                        gint               page_nr)
{
  gdouble paper_w = gtk_print_context_get_width  (context);
  gdouble paper_h = gtk_print_context_get_height (context);

  if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("table_classification_toggletoolbutton"))))
  {
    Module::OnDrawPage (operation,
                        context,
                        page_nr);
    return;
  }

  if (_print_full_table)
  {
    cairo_t         *cr       = gtk_print_context_get_cairo_context (context);
    guint            x_page   = page_nr % _print_nb_x_pages;
    guint            y_page   = page_nr / _print_nb_x_pages;
    gdouble          header_h = (PRINT_HEADER_HEIGHT+2) * paper_w  / 100;
    GooCanvasBounds  bounds;

    if (y_page == 0)
    {
      Module::OnDrawPage (operation,
                          context,
                          page_nr);
    }

    cairo_save (cr);

    cairo_scale (cr,
                 _print_scale,
                 _print_scale);

    bounds.x1 = paper_w*(x_page)   / _print_scale;
    bounds.y1 = paper_h*(y_page)   / _print_scale;
    bounds.x2 = paper_w*(x_page+1) / _print_scale;
    bounds.y2 = paper_h*(y_page+1) / _print_scale;

    if (y_page == 0)
    {
      cairo_translate (cr,
                       -bounds.x1,
                       header_h/_print_scale);

      bounds.y2 -= header_h/_print_scale;
    }
    else
    {
      cairo_translate (cr,
                       -bounds.x1,
                       (header_h - paper_h*(y_page)) / _print_scale);

      bounds.y1 -= header_h/_print_scale;
      bounds.y2 -= header_h/_print_scale;
    }

    goo_canvas_render (GetCanvas (),
                       cr,
                       &bounds,
                       1.0);

    cairo_restore (cr);
  }
  else
  {
    cairo_t   *cr      = gtk_print_context_get_cairo_context (context);
    GooCanvas *canvas  = Canvas::CreatePrinterCanvas (context);
    GSList    *current_match;

    cairo_save (cr);

    current_match = g_slist_nth (_match_to_print,
                                 4*page_nr);
    for (guint i = 0; current_match && (i < 4); i++)
    {
      GooCanvasItem *group;

      group = goo_canvas_group_new (goo_canvas_get_root_item (canvas),
                                    NULL);

      {
        cairo_matrix_t matrix;

        cairo_matrix_init_translate (&matrix,
                                     0.0,
                                     i*25.0 * paper_h/paper_w);

        g_object_set_data (G_OBJECT (operation), "operation_matrix", (void *) &matrix);

        Module::OnDrawPage (operation,
                            context,
                            page_nr);
      }

      {
        GString       *image;
        Match         *match  = (Match *) current_match->data;
        Player        *A      = match->GetPlayerA ();
        Player        *B      = match->GetPlayerB ();
        gdouble        offset = i * (25.0 * paper_h/paper_w) + (PRINT_HEADER_HEIGHT + 10.0);
        GooCanvasItem *item;
        gdouble        name_width;
        gchar         *match_number = g_strdup_printf (gettext ("Match %s"), (const char *) match->GetData (this, "number"));

        match->SetData (this, "printed", (void *) TRUE);

        {
          gchar *font   = g_strdup_printf ("Sans Bold %fpx", 3.5/2.0*(PRINT_FONT_HEIGHT));
          gchar *decimal = strchr (font, ',');

          if (decimal)
          {
            *decimal = '.';
          }

          goo_canvas_text_new (group,
                               match_number,
                               0.0,
                               0.0 + offset - 6.0,
                               -1.0,
                               GTK_ANCHOR_W,
                               "fill-color", "grey",
                               "font", font,
                               NULL);
          g_free (match_number);
          g_free (font);
        }

        {
          GooCanvasBounds  bounds;
          gchar           *font = g_strdup_printf ("Sans Bold %fpx", PRINT_FONT_HEIGHT);
          gchar *decimal = strchr (font, ',');

          if (decimal)
          {
            *decimal = '.';
          }

          image = GetPlayerImage (A);
          item = goo_canvas_text_new (group,
                                      image->str,
                                      0.0,
                                      0.0 + offset,
                                      -1.0,
                                      GTK_ANCHOR_W,
                                      "font", font,
                                      NULL);
          g_string_free (image,
                         TRUE);
          goo_canvas_item_get_bounds (item,
                                      &bounds);
          name_width = bounds.x2 - bounds.x1;

          image = GetPlayerImage (B);
          item = goo_canvas_text_new (group,
                                      image->str,
                                      0.0,
                                      7.5 + offset,
                                      -1.0,
                                      GTK_ANCHOR_W,
                                      "font", font,
                                      NULL);
          g_string_free (image,
                         TRUE);

          g_free (font);

          goo_canvas_item_get_bounds (item,
                                      &bounds);
          if (name_width < bounds.x2 - bounds.x1)
          {
            name_width = bounds.x2 - bounds.x1;
          }
        }

        {
          gdouble x;

          goo_canvas_convert_to_item_space (canvas,
                                            item,
                                            &x,
                                            &name_width);
        }

        {
          gchar *font = g_strdup_printf ("Sans Bold %fpx", 1.5/2.0*(PRINT_FONT_HEIGHT));
          gchar *decimal = strchr (font, ',');

          if (decimal)
          {
            *decimal = '.';
          }

          for (guint j = 0; j < _max_score->_value; j++)
          {
            gchar *number;

            number = g_strdup_printf ("%d", j+1);
            goo_canvas_text_new (group,
                                 number,
                                 name_width + j*2.5 + PRINT_FONT_HEIGHT/2,
                                 0.0 + offset,
                                 -1.0,
                                 GTK_ANCHOR_CENTER,
                                 "font", font,
                                 "fill-color", "grey",
                                 NULL);
            goo_canvas_rect_new (group,
                                 name_width + j*2.5,
                                 0.0 + offset - PRINT_FONT_HEIGHT/2.0,
                                 PRINT_FONT_HEIGHT,
                                 PRINT_FONT_HEIGHT,
                                 "line-width", 0.25,
                                 NULL);

            goo_canvas_text_new (group,
                                 number,
                                 name_width + j*2.5 + PRINT_FONT_HEIGHT/2,
                                 7.5 + offset,
                                 -1.0,
                                 GTK_ANCHOR_CENTER,
                                 "font", font,
                                 "fill-color", "grey",
                                 NULL);
            goo_canvas_rect_new (group,
                                 name_width + j*2.5,
                                 7.5 + offset - PRINT_FONT_HEIGHT/2.0,
                                 PRINT_FONT_HEIGHT,
                                 PRINT_FONT_HEIGHT,
                                 "line-width", 0.25,
                                 NULL);

            g_free (number);
          }
          g_free (font);
        }

        {
          gdouble score_size = 4.0;

          goo_canvas_rect_new (group,
                               name_width + _max_score->_value*2.5 + score_size,
                               0.0 + offset - score_size/2.0,
                               score_size,
                               score_size,
                               "line-width", 0.3,
                               NULL);
          goo_canvas_rect_new (group,
                               name_width + _max_score->_value*2.5 + score_size,
                               7.5 + offset - score_size/2.0,
                               score_size,
                               score_size,
                               "line-width", 0.3,
                               NULL);


          goo_canvas_rect_new (group,
                               name_width + _max_score->_value*2.5 + 2.5*score_size,
                               0.0 + offset - score_size/2.0,
                               score_size*5,
                               score_size,
                               "fill-color", "Grey85",
                               "line-width", 0.0,
                               NULL);
          goo_canvas_text_new (group,
                               gettext ("Signature"),
                               name_width + _max_score->_value*2.5 + 4.5*score_size,
                               0.0 + offset,
                               -1,
                               GTK_ANCHOR_CENTER,
                               "fill-color", "Grey95",
                               "font", "Sans bold 3.5px",
                               NULL);
          goo_canvas_rect_new (group,
                               name_width + _max_score->_value*2.5 + 2.5*score_size,
                               7.5 + offset - score_size/2.0,
                               score_size*5,
                               score_size,
                               "fill-color", "Grey85",
                               "line-width", 0.0,
                               NULL);
          goo_canvas_text_new (group,
                               gettext ("Signature"),
                               name_width + _max_score->_value*2.5 + 4.5*score_size,
                               7.5 + offset,
                               -1,
                               GTK_ANCHOR_CENTER,
                               "fill-color", "Grey95",
                               "font", "Sans bold 3.5px",
                               NULL);
        }
      }

      Canvas::FitToContext (group,
                            context);

      current_match = g_slist_next (current_match);
    }

    goo_canvas_render (canvas,
                       cr,
                       NULL,
                       1.0);
    gtk_widget_destroy (GTK_WIDGET (canvas));

    cairo_restore (cr);
  }
}

// --------------------------------------------------------------------------------
void Table::OnZoom (gdouble value)
{
  goo_canvas_set_scale (GetCanvas (),
                        value);
}

// --------------------------------------------------------------------------------
void Table::OnPrint ()
{
  if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("table_classification_toggletoolbutton"))))
  {
    Classification *classification = GetClassification ();

    if (classification)
    {
      classification->Print (gettext ("Table round classification"));
    }
  }
  else if (gtk_dialog_run (GTK_DIALOG (_print_dialog)) == GTK_RESPONSE_OK)
  {
    GtkWidget *w = _glade->GetWidget ("table_radiobutton");

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
    {
      _print_full_table = TRUE;
      Print (gettext ("Table"));
    }
    else
    {
      GtkWidget *w         = _glade->GetWidget ("all_radiobutton");
      gboolean   all_sheet = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));

      LookForMatchToPrint (0,
                           all_sheet);
      _print_full_table = FALSE;
      Print (gettext ("Score sheet"));
    }
  }

  gtk_widget_hide (_print_dialog);
}

// --------------------------------------------------------------------------------
gboolean Table::OnPrintLevel (GooCanvasItem  *item,
                              GooCanvasItem  *target_item,
                              GdkEventButton *event,
                              Table          *table)
{
  guint level_to_print = (guint) g_object_get_data (G_OBJECT (item), "level_to_print");
  gchar *title         = g_strdup_printf (gettext ("%s: score sheets printing"), table->GetLevelImage (level_to_print));

  gtk_window_set_title (GTK_WINDOW (table->_level_print_dialog),
                        title);
  g_free (title);

  if (gtk_dialog_run (GTK_DIALOG (table->_level_print_dialog)) == GTK_RESPONSE_OK)
  {
    GtkWidget *w         = table->_glade->GetWidget ("level_pending_radiobutton");
    gboolean   all_sheet = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));

    table->LookForMatchToPrint (level_to_print,
                                all_sheet);
    table->_print_full_table = FALSE;
    table->Print (gettext ("Score sheet"));
  }

  gtk_widget_hide (table->_level_print_dialog);

  return TRUE;
}

// --------------------------------------------------------------------------------
gboolean Table::OnPrintMatch (GooCanvasItem  *item,
                              GooCanvasItem  *target_item,
                              GdkEventButton *event,
                              Table          *table)
{
  if (table->_match_to_print)
  {
    g_slist_free (table->_match_to_print);
    table->_match_to_print = NULL;
  }

  table->_match_to_print = g_slist_prepend (table->_match_to_print,
                                            g_object_get_data (G_OBJECT (item), "match_to_print"));
  table->_print_full_table = FALSE;
  table->Print (gettext ("Score sheet"));

  return TRUE;
}

// --------------------------------------------------------------------------------
void Table::OnStatusChanged (GtkComboBox *combo_box)
{
  GtkTreeIter          iter;
  gchar               *code;
  Match               *match  = (Match *)  g_object_get_data (G_OBJECT (combo_box), "match_for_status");
  Player              *player = (Player *) g_object_get_data (G_OBJECT (combo_box), "player_for_status");
  Player::AttributeId  status_attr_id = Player::AttributeId ("status", GetDataOwner ());

  gtk_combo_box_get_active_iter (combo_box,
                                 &iter);
  gtk_tree_model_get (GetStatusModel (),
                      &iter,
                      AttributeDesc::DISCRETE_XML_IMAGE, &code,
                      -1);

  player->SetAttributeValue (&status_attr_id,
                             code);
  if (code && *code !='Q')
  {
    match->DropPlayer (player);
  }
  else
  {
    match->RestorePlayer (player);
  }
  OnAttrListUpdated ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_table_filter_toolbutton_clicked (GtkWidget *widget,
                                                                    Object    *owner)
{
  Table *t = dynamic_cast <Table *> (owner);

  t->OnFilterClicked ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_from_table_combobox_changed (GtkWidget *widget,
                                                                Object    *owner)
{
  Table *t = dynamic_cast <Table *> (owner);

  t->OnFromTableComboboxChanged ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_table_classification_toggletoolbutton_toggled (GtkWidget *widget,
                                                                                  Object    *owner)
{
  Table *t = dynamic_cast <Table *> (owner);

  t->ToggleClassification (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (widget)));
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_table_stuff_toolbutton_clicked (GtkWidget *widget,
                                                                   Object    *owner)
{
  Table *t = dynamic_cast <Table *> (owner);

  t->OnStuffClicked ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_input_toolbutton_toggled (GtkWidget *widget,
                                                             Object    *owner)
{
  Table *t = dynamic_cast <Table *> (owner);

  t->OnInputToggled (widget);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_quick_search_combobox_changed (GtkWidget *widget,
                                                                  Object    *owner)
{
  Table *t = dynamic_cast <Table *> (owner);

  t->OnSearchMatch ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_zoom_scalebutton_value_changed (GtkWidget *widget,
                                                                   gdouble    value,
                                                                   Object    *owner)
{
  Table *t = dynamic_cast <Table *> (owner);

  t->OnZoom (value);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_match_sheet_radiobutton_toggled (GtkWidget *widget,
                                                                    Object    *owner)
{
  Table *t = dynamic_cast <Table *> (owner);

  t->OnMatchSheetToggled (widget);
}

// --------------------------------------------------------------------------------
void Table::on_status_changed (GtkComboBox *combo_box,
                               Table       *table)
{
  table->OnStatusChanged (combo_box);
}
