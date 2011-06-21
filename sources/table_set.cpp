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
#include "table_supervisor.hpp"
#include "table_set.hpp"

const gdouble TableSet::_score_rect_size = 30.0;
const gdouble TableSet::_table_spacing   = 10.0;

typedef enum
{
  FROM_NAME_COLUMN,
  FROM_STATUS_COLUMN
} FromColumnId;

typedef enum
{
  DISPLAY_NAME_COLUMN
} DisplayColumnId;

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
TableSet::TableSet (TableSupervisor *supervisor,
                    gchar           *id,
                    GtkWidget       *control_container,
                    guint            first_place)
: CanvasModule ("table.glade")
{
  Module *supervisor_module = dynamic_cast <Module *> (supervisor);

  _supervisor        = supervisor;
  _filter            = supervisor_module->GetFilter ();
  _control_container = control_container;
  _main_table        = NULL;
  _tree_root         = NULL;
  _tables            = NULL;
  _match_to_print    = NULL;
  _nb_tables         = 0;
  _locked            = FALSE;
  _id                = id;
  _attendees         = NULL;
  _name              = NULL;
  _has_error         = FALSE;
  _is_over           = FALSE;
  _first_place       = first_place;
  _loaded            = FALSE;

  _status_cbk_data = NULL;
  _status_cbk      = NULL;

  SetDataOwner (supervisor_module);
  _score_collector = NULL;

  _max_score = supervisor->GetMaxScore ();

  {
    GtkWidget *content_area;

    _table_print_dialog = gtk_message_dialog_new_with_markup (NULL,
                                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                                              GTK_MESSAGE_QUESTION,
                                                              GTK_BUTTONS_OK_CANCEL,
                                                              gettext ("<b><big>Which score sheets of the selected table do you want to print?</big></b>"));

    content_area = gtk_dialog_get_content_area (GTK_DIALOG (_table_print_dialog));

    gtk_widget_reparent (_glade->GetWidget ("print_table_dialog_vbox"),
                         content_area);

    gtk_widget_set_sensitive (_glade->GetWidget ("match_sheet_vbox"),
                              FALSE);
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
    GtkWidget *content_area;

    _preview_dialog = gtk_dialog_new_with_buttons (gettext ("Table printing"),
                                                   NULL,
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_STOCK_PRINT,  GTK_RESPONSE_OK,
                                                   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                   NULL);

    content_area = gtk_dialog_get_content_area (GTK_DIALOG (_preview_dialog));

    gtk_widget_reparent (_glade->GetWidget ("preview_dialog-vbox"),
                         content_area);
  }

  {
    _quick_score_collector = new ScoreCollector (this,
                                                 (ScoreCollector::OnNewScore_cbk) &TableSet::OnNewScore);

    _quick_score_A = GetQuickScore ("fencerA_hook");
    _quick_score_B = GetQuickScore ("fencerB_hook");

    _quick_score_collector->SetNextCollectingPoint (_quick_score_A,
                                                    _quick_score_B);
    _quick_score_collector->SetNextCollectingPoint (_quick_score_B,
                                                    _quick_score_A);
  }

  {
    _from_widget = _glade->GetWidget ("from_table_combobox");

    g_object_ref (_from_widget);
    gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (_from_widget)),
                          _from_widget);
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
TableSet::~TableSet ()
{
  DeleteTree ();

  Object::TryToRelease (_quick_score_collector);
  Object::TryToRelease (_score_collector);

  g_free (_name);

  if (_attendees)
  {
    g_slist_free (_attendees);
  }

  gtk_list_store_clear (_from_table_liststore);
  gtk_tree_store_clear (_quick_search_treestore);

  if (_match_to_print)
  {
    g_slist_free (_match_to_print);
  }

  gtk_widget_destroy (_print_dialog);
  gtk_widget_destroy (_preview_dialog);
  gtk_widget_destroy (_table_print_dialog);

  g_free (_id);

  g_object_unref (_from_widget);
}

// --------------------------------------------------------------------------------
void TableSet::SetAttendees (GSList *attendees)
{
  _attendees = attendees;
  Garnish ();
  RefreshTableStatus ();
}

// --------------------------------------------------------------------------------
void TableSet::SetQuickSearchRendererSensitivity (GtkCellLayout   *cell_layout,
                                                  GtkCellRenderer *cell,
                                                  GtkTreeModel    *tree_model,
                                                  GtkTreeIter     *iter,
                                                  TableSet        *table_set)
{
  gboolean sensitive = !gtk_tree_model_iter_has_child (tree_model, iter);

  g_object_set (cell, "sensitive", sensitive, NULL);
}

// --------------------------------------------------------------------------------
GooCanvasItem *TableSet::GetQuickScore (const gchar *container)
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
void TableSet::RefreshTableStatus ()
{
  if (_tree_root)
  {
    _has_error = FALSE;
    _is_over   = TRUE;

    for (guint i = 0; i < _nb_tables; i ++)
    {
      _tables[i]->_is_over   = TRUE;
      _tables[i]->_has_error = FALSE;
      if (_tables[i]->_status_item)
      {
        WipeItem (_tables[i]->_status_item);
        _tables[i]->_status_item = NULL;
      }
    }

    g_node_traverse (_tree_root,
                     G_POST_ORDER,
                     G_TRAVERSE_ALL,
                     -1,
                     (GNodeTraverseFunc) UpdateTableStatus,
                     this);

    for (guint t = 0; t < _nb_tables; t++)
    {
      GtkTreeIter  iter;
      gchar       *icon;
      Table       *table = _tables[t];

      if (table->_has_error)
      {
        icon = g_strdup (GTK_STOCK_DIALOG_WARNING);
        _has_error = TRUE;
        _is_over   = FALSE;
      }
      else if (table->_is_over == TRUE)
      {
        icon = g_strdup (GTK_STOCK_APPLY);
      }
      else
      {
        icon = g_strdup (GTK_STOCK_EXECUTE);
        _is_over = FALSE;
      }

      if (    IsPlugged ()
           && table->IsDisplayed ()
           && (table->GetSize () > 1))
      {
        table->_status_item = Canvas::PutStockIconInTable (table->_header_item,
                                                           icon,
                                                           0, 0);
      }

      if (gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (_from_table_liststore),
                                         &iter,
                                         NULL,
                                         _nb_tables-t-1) == TRUE)
      {
        gtk_list_store_set (_from_table_liststore, &iter,
                            FROM_STATUS_COLUMN, icon,
                            -1);
      }

      g_free (icon);
    }

    if (_status_cbk)
    {
      _status_cbk (this,
                   _status_cbk_data);
    }
  }
  MakeDirty ();
}

// --------------------------------------------------------------------------------
void TableSet::SetStatusCbk (StatusCbk  cbk,
                             void      *data)
{
  _status_cbk_data = data;
  _status_cbk      = cbk;

  if (_status_cbk)
  {
    _status_cbk (this,
                 _status_cbk_data);
  }
}

// --------------------------------------------------------------------------------
void TableSet::Display ()
{
  Wipe ();

  if (_tree_root)
  {
    _main_table = goo_canvas_table_new (GetRootItem (),
                                        "column-spacing", _table_spacing,
                                        NULL);

    // Header
    for (guint t = 0; t < _nb_tables; t++)
    {
      Table *table = _tables[t];

      if (table->IsDisplayed ())
      {
        table->_header_item = goo_canvas_table_new (_main_table,
                                                    NULL);

        {
          GooCanvasItem *text_item;
          gchar         *text  = table->GetImage ();

          text_item = Canvas::PutTextInTable (table->_header_item,
                                              text,
                                              0,
                                              1);
          g_object_set (G_OBJECT (text_item),
                        "font", "Sans bold 18px",
                        NULL);
          g_free (text);
        }

        if (table->GetSize () > 1)
        {
          GooCanvasItem *print_item;

          print_item = Canvas::PutStockIconInTable (table->_header_item,
                                                    GTK_STOCK_PRINT,
                                                    1,
                                                    1);
          Canvas::SetTableItemAttribute (print_item, "x-align", 0.5);

          g_object_set_data (G_OBJECT (print_item), "table_to_print", table);
          g_signal_connect (print_item, "button-release-event",
                            G_CALLBACK (OnPrintTable), this);
        }

        Canvas::PutInTable (_main_table,
                            table->_header_item,
                            0,
                            table->GetColumn ());

        Canvas::SetTableItemAttribute (table->_header_item, "x-align", 0.5);
      }
    }

    g_node_traverse (_tree_root,
                     G_POST_ORDER,
                     G_TRAVERSE_ALL,
                     -1,
                     (GNodeTraverseFunc) FillInNode,
                     this);

    RefreshTableStatus ();
    DrawAllConnectors ();
  }
}

// --------------------------------------------------------------------------------
gboolean TableSet::IsOver ()
{
  return _is_over;
}
// --------------------------------------------------------------------------------
gboolean TableSet::HasError ()
{
  return _has_error;
}

// --------------------------------------------------------------------------------
void TableSet::SetName (gchar *name)
{
  _name = g_strdup (name);
}

// --------------------------------------------------------------------------------
gchar *TableSet::GetName ()
{
  return _name;
}

// --------------------------------------------------------------------------------
void TableSet::Garnish ()
{
  DeleteTree ();
  CreateTree ();
}

// --------------------------------------------------------------------------------
guint TableSet::GetNbTables ()
{
  return _nb_tables;
}

// --------------------------------------------------------------------------------
Table *TableSet::GetTable (guint size)
{
  for (guint t = 0; t < _nb_tables; t++)
  {
    if (_tables[t]->GetSize () == size)
    {
      return _tables[t];
    }
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void TableSet::Load (xmlNode *xml_node)
{
  LoadNode (xml_node);

  RefreshTableStatus ();

  for (guint t = 1; t < _nb_tables; t++)
  {
    Table *table = _tables[t];

    if (table->_is_over)
    {
      _supervisor->OnTableOver (this,
                                table);
    }
  }
}

// --------------------------------------------------------------------------------
void TableSet::LoadNode (xmlNode *xml_node)
{
  for (xmlNode *n = xml_node; n != NULL; n = n->next)
  {
    static Table *table = NULL;

    if (n->type == XML_ELEMENT_NODE)
    {
      if ((_loaded == FALSE) && (strcmp ((char *) n->name, "SuiteDeTableaux") == 0))
      {
        _loaded = TRUE;
      }
      else if (strcmp ((char *) n->name, "Tableau") == 0)
      {
        gchar *prop;

        prop = (gchar *) xmlGetProp (n, BAD_CAST "Taille");
        if (prop)
        {
          table = GetTable (atoi (prop));

          if (table)
          {
            table->Load (n);
          }
        }
      }
      else
      {
        return;
      }
    }
    LoadNode (n->children);
  }
}

// --------------------------------------------------------------------------------
void TableSet::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST "SuiteDeTableaux");
  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "ID",
                                     "%s", _id);
  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "Titre",
                                     "%s", _name);
  if (_nb_tables)
  {
    xmlTextWriterWriteFormatAttribute (xml_writer,
                                       BAD_CAST "NbDeTableaux",
                                       "%d", _nb_tables-1);
  }

  if (_tree_root)
  {
    for (guint t = _nb_tables-1; t > 0; t--)
    {
      Table *table = _tables[t];

      table->Save (xml_writer);
    }
  }
  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void TableSet::OnNewScore (ScoreCollector *score_collector,
                           CanvasModule   *client,
                           Match          *match,
                           Player         *player)
{
  TableSet *table_set = dynamic_cast <TableSet *> (client);
  GNode    *node      = (GNode *) match->GetPtrData (table_set, "node");

  FillInNode (node,
              table_set);

  if (score_collector == table_set->_quick_score_collector)
  {
    table_set->_score_collector->Refresh (match);
  }
  else
  {
    table_set->_quick_score_collector->Refresh (match);
  }

  if (node->parent)
  {
    FillInNode (node->parent,
                table_set);
  }

  table_set->RefreshTableStatus ();
  table_set->DrawAllConnectors ();

  {
    Table *table = (Table *) match->GetPtrData (table_set, "table");

    if (table->_is_over)
    {
      table_set->_supervisor->OnTableOver (table_set,
                                           table);
    }
  }
}

// --------------------------------------------------------------------------------
void TableSet::DeleteTree ()
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

  for (guint i = 0; i < _nb_tables; i++)
  {
    _tables[i]->Release ();
  }
  g_free (_tables);
  _nb_tables = 0;
  _tables    = NULL;
}

// --------------------------------------------------------------------------------
void TableSet::CreateTree ()
{
  guint nb_players = g_slist_length (_attendees);

  for (guint i = 0; i < 32; i++)
  {
    guint bit_cursor;

    bit_cursor = 1;
    bit_cursor = bit_cursor << i;
    if (bit_cursor >= nb_players)
    {
      _nb_tables = i++;
      break;
    }
  }

  _nb_tables++;

  _tables = (Table **) g_malloc (_nb_tables * sizeof (Table *));
  for (guint t = 0; t < _nb_tables; t++)
  {
    // Create the tables
    {
      _tables[t] = new Table (this,
                              1 << t,
                              _nb_tables - t-1);

      if (t > 0)
      {
        _tables[t]->SetRightTable (_tables[t-1]);

        // Add an entry for each table in the the quick search
        {
          GtkTreeIter table_iter;

          gtk_tree_store_prepend (_quick_search_treestore,
                                  &table_iter,
                                  NULL);
          gtk_tree_store_set (_quick_search_treestore, &table_iter,
                              QUICK_MATCH_NAME_COLUMN, _tables[t]->GetImage (),
                              QUICK_MATCH_VISIBILITY_COLUMN, 1,
                              -1);
        }
      }
    }
  }

  AddFork (NULL);

  {
    g_signal_handlers_disconnect_by_func (_glade->GetWidget ("from_table_combobox"),
                                          (void *) on_from_table_combobox_changed,
                                          (Object *) this);
    gtk_list_store_clear (_from_table_liststore);

    for (guint t = 1; t < _nb_tables; t++)
    {
      Table        *table = _tables[t];
      gchar        *text = table->GetImage ();
      GtkTreeIter   iter;

      gtk_list_store_prepend (_from_table_liststore,
                              &iter);

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
void TableSet::DrawAllConnectors ()
{
  if (_tree_root)
  {
    g_node_traverse (_tree_root,
                     G_POST_ORDER,
                     G_TRAVERSE_ALL,
                     -1,
                     (GNodeTraverseFunc) DrawConnector,
                     this);
  }
}

// --------------------------------------------------------------------------------
gboolean TableSet::WipeNode (GNode    *node,
                             TableSet *table_set)
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
gboolean TableSet::DeleteCanvasTable (GNode    *node,
                                      TableSet *table_set)
{
  NodeData *data = (NodeData *) node->data;

  table_set->_score_collector->RemoveCollectingPoints (data->_match);

  data->_player_item = NULL;
  data->_print_item  = NULL;
  data->_connector   = NULL;

  data->_canvas_table = NULL;

  {
    GNode *parent = node->parent;

    if (parent)
    {
      NodeData *parent_data = (NodeData *) parent->data;

      if (parent_data->_match)
      {
        parent_data->_match->RemoveData (table_set, "A_collecting_point");
        parent_data->_match->RemoveData (table_set, "B_collecting_point");
      }
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean TableSet::UpdateTableStatus (GNode    *node,
                                      TableSet *table_set)
{
  NodeData *data       = (NodeData *) node->data;
  Table    *left_table = data->_table->GetLeftTable ();

  if (data->_match && left_table)
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
        left_table->_has_error = TRUE;
      }

      left_table->_is_over = FALSE;
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean TableSet::DrawConnector (GNode    *node,
                                  TableSet *table_set)
{
  NodeData *data = (NodeData *) node->data;

  if (data->_table->IsDisplayed () && data->_match)
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
      data->_connector = goo_canvas_polyline_new (table_set->GetRootItem (),
                                                  FALSE,
                                                  2,
                                                  bounds.x1 - _table_spacing/2, bounds.y2,
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

      data->_connector = goo_canvas_polyline_new (table_set->GetRootItem (),
                                                  FALSE,
                                                  3,
                                                  bounds.x1 - _table_spacing/2, bounds.y2,
                                                  parent_bounds.x1 - _table_spacing/2, bounds.y2,
                                                  parent_bounds.x1 - _table_spacing/2, parent_bounds.y2,
                                                  "line-width", 2.0,
                                                  NULL);
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean TableSet::FillInNode (GNode    *node,
                               TableSet *table_set)
{
  GNode    *parent = node->parent;
  NodeData *data   = (NodeData *) node->data;

  if (data->_match && data->_table->IsDisplayed ())
  {
    Player *winner = data->_match->GetWinner ();

    WipeNode (node,
              table_set);

    if (data->_canvas_table == NULL)
    {
      guint row = data->_table->GetRow (data->_table_index) + 1;

      data->_canvas_table = goo_canvas_table_new (table_set->_main_table,
                                                  "column-spacing", table_set->_table_spacing,
                                                  NULL);
      Canvas::SetTableItemAttribute (data->_canvas_table, "x-fill", 1U);
      //Canvas::SetTableItemAttribute (data->_canvas_table, "x-expand", 1U);

      Canvas::PutInTable (table_set->_main_table,
                          data->_canvas_table,
                          row,
                          data->_table->GetColumn ());
    }

    {
      gchar *match_name = data->_match->GetName ();

      if (match_name)
      {
        GooCanvasItem *number_item = Canvas::PutTextInTable (data->_canvas_table,
                                                             match_name,
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
                        G_CALLBACK (OnPrintMatch), table_set);
    }

    if (winner)
    {
      GString *string = table_set->GetPlayerImage (winner);

      data->_player_item = Canvas::PutTextInTable (data->_canvas_table,
                                                   string->str,
                                                   0,
                                                   2);
      Canvas::SetTableItemAttribute (data->_player_item, "y-align", 0.5);

      g_string_free (string,
                     TRUE);

    }

    if (parent && winner)
    {
      GooCanvasItem *goo_rect;
      GooCanvasItem *score_text;
      NodeData      *parent_data = (NodeData *) parent->data;
      guint          position    = g_node_child_position (parent, node);

      table_set->SetPlayerToMatch (parent_data->_match,
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

        Canvas::PutInTable (data->_canvas_table,
                            goo_rect,
                            0,
                            3);
        Canvas::SetTableItemAttribute (goo_rect, "y-align", 0.5);
      }

      // Status arrow
      if (   (parent_data->_match->GetWinner () == NULL)
             || (   parent_data->_match->GetPlayerA ()
                    && parent_data->_match->GetPlayerB ()))
      {
        GooCanvasItem *goo_item;

        goo_item = Canvas::PutIconInTable (data->_canvas_table,
                                           "resources/glade/arrow.png",
                                           0,
                                           3);
        Canvas::SetTableItemAttribute (goo_item, "x-align", 1.0);
        Canvas::SetTableItemAttribute (goo_item, "y-align", 0.0);

        g_object_set_data (G_OBJECT (goo_item), "TableSet::node", node);
        g_signal_connect (goo_item, "button_press_event",
                          G_CALLBACK (on_status_arrow_press), table_set);
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
                            3);
        Canvas::SetTableItemAttribute (score_text, "x-align", 0.5);
        Canvas::SetTableItemAttribute (score_text, "y-align", 0.5);
      }

      {
        Player *A = parent_data->_match->GetPlayerA ();
        Player *B = parent_data->_match->GetPlayerB ();

        if (   (parent_data->_match->GetWinner () == NULL)
               || ((A != NULL) && (B != NULL)))
        {
          if (winner == A)
          {
            parent_data->_match->SetData (table_set, "A_collecting_point", goo_rect);
          }
          if (winner == B)
          {
            parent_data->_match->SetData (table_set, "B_collecting_point", goo_rect);
          }

          table_set->_score_collector->AddCollectingPoint (goo_rect,
                                                           score_text,
                                                           parent_data->_match,
                                                           winner);
          table_set->_score_collector->AddCollectingTrigger (data->_player_item);

          if (A && B)
          {
            table_set->_score_collector->SetNextCollectingPoint ((GooCanvasItem *) parent_data->_match->GetPtrData (table_set,
                                                                                                                    "A_collecting_point"),
                                                                 (GooCanvasItem *) parent_data->_match->GetPtrData (table_set,
                                                                                                                    "B_collecting_point"));
            table_set->_score_collector->SetNextCollectingPoint ((GooCanvasItem *) parent_data->_match->GetPtrData (table_set,
                                                                                                                    "B_collecting_point"),
                                                                 (GooCanvasItem *) parent_data->_match->GetPtrData (table_set,
                                                                                                                    "A_collecting_point"));
          }
        }
      }

      // Status icon
      if (parent_data->_match->IsDropped ())
      {
        Player::AttributeId  attr_id ("status", table_set->GetDataOwner ());
        gchar               *status = winner->GetAttribute (&attr_id)->GetStrValue ();

        if ((status[0] == 'E') || (status[0] == 'A'))
        {
          AttributeDesc *attr_desc = AttributeDesc::GetDesc ("status");
          GooCanvasItem *status_item;

          status_item = Canvas::PutIconInTable (data->_canvas_table,
                                                attr_desc->GetDiscreteIcon (status[0]),
                                                0,
                                                3);

          Canvas::SetTableItemAttribute (status_item, "x-align", 0.5);
          Canvas::SetTableItemAttribute (status_item, "y-align", 0.5);
        }
      }
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean TableSet::DeleteNode (GNode    *node,
                               TableSet *table_set)
{
  NodeData *data = (NodeData *) node->data;

  Object::TryToRelease (data->_match);

  return FALSE;
}

// --------------------------------------------------------------------------------
void TableSet::AddFork (GNode *to)
{
  GNode    *node;
  NodeData *to_data    = NULL;
  NodeData *data       = new NodeData;
  Table    *left_table;

  if (to)
  {
    to_data = (NodeData *) to->data;
  }

  if (to == NULL)
  {
    data->_expected_winner_rank = 1;
    data->_table                = _tables[0];
    data->_table_index          = 0;
    node = g_node_new (data);
    _tree_root = node;
  }
  else
  {
    data->_table = to_data->_table->GetLeftTable ();

    if (g_node_n_children (to) == 0)
    {
      data->_table_index = to_data->_table_index*2;
    }
    else
    {
      data->_table_index = to_data->_table_index*2 + 1;
    }

    if (g_node_n_children (to) != (to_data->_expected_winner_rank % 2))
    {
      data->_expected_winner_rank = to_data->_expected_winner_rank;
    }
    else
    {
      data->_expected_winner_rank = (data->_table->GetSize () + 1) - to_data->_expected_winner_rank;
    }
    node = g_node_append_data (to, data);
  }

  data->_canvas_table = NULL;
  data->_player_item  = NULL;
  data->_print_item   = NULL;
  data->_connector    = NULL;
  data->_match = new Match (_max_score);
  data->_match->SetData (this, "node", node);

  {
    gchar *name_space = g_strdup_printf ("%d.", data->_table->GetSize ()*2);

    data->_match->SetNameSpace (name_space);
    g_free (name_space);
  }

  left_table = data->_table->GetLeftTable ();
  if (left_table)
  {
    GtkTreeIter table_iter;

    // Get the table entry in the quick search
    {
      GtkTreePath *path = gtk_tree_path_new_from_indices (data->_table->GetColumn () - 1,
                                                          -1);

      gtk_tree_model_get_iter (GTK_TREE_MODEL (_quick_search_treestore),
                               &table_iter,
                               path);

      gtk_tree_path_free (path);
    }

    // Add a sub entry for the match in the quick search
    {
      GtkTreeIter  iter;
      gint        *indices;
      GtkTreePath *path;

      gtk_tree_store_append (_quick_search_treestore,
                             &iter,
                             &table_iter);

      path = gtk_tree_model_get_path (GTK_TREE_MODEL (_quick_search_treestore),
                                      &iter);
      indices = gtk_tree_path_get_indices (path);

      data->_match->SetData (this,
                             "quick_search_path", path, (GDestroyNotify) gtk_tree_path_free);

      data->_match->SetNumber (indices[1]+1);

      left_table->ManageMatch (data->_match);
    }

    data->_match->SetData (this,
                           "table", data->_table->GetLeftTable ());

    AddFork (node);
    AddFork (node);
  }
  else
  {
    Player *player = (Player *) g_slist_nth_data (_attendees,
                                                  data->_expected_winner_rank - 1);

    if (player)
    {
      data->_match->SetPlayerA (player);
      data->_match->SetPlayerB (NULL);

      if (to_data)
      {
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
    else if (to_data)
    {
      data->_table->DropMatch (data->_match);
      data->_match->Release ();
      data->_match = NULL;

      if (g_node_child_position (to, node) == 0)
      {
        to_data->_match->SetPlayerA (NULL);
      }
      else
      {
        GNode    *A_node = g_node_first_child (to);
        NodeData *A_data = (NodeData *) A_node->data;

        to_data->_match->SetPlayerA (A_data->_match->GetWinner ());
        to_data->_match->SetPlayerB (NULL);
      }
    }
  }
}

// --------------------------------------------------------------------------------
void TableSet::Wipe ()
{
  {
    Object::TryToRelease (_score_collector);

    _score_collector = new ScoreCollector (this,
                                           (ScoreCollector::OnNewScore_cbk) &TableSet::OnNewScore);

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

  for (guint i = 0; i < _nb_tables; i ++)
  {
    _tables[i]->_status_item = NULL;
  }
}

// --------------------------------------------------------------------------------
void TableSet::LookForMatchToPrint (Table    *table_to_print,
                                    gboolean  all_sheet)
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
          Table *table = (Table *) match->GetPtrData (this, "table");

          if ((table_to_print == NULL) || (table_to_print == table))
          {
            gboolean already_printed = match->GetUIntData (this, "printed");

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
void TableSet::Lock ()
{
  _locked = TRUE;

  if (_score_collector)
  {
    _score_collector->Lock ();
  }
}

// --------------------------------------------------------------------------------
void TableSet::UnLock ()
{
  _locked = FALSE;

  if (_score_collector)
  {
    _score_collector->UnLock ();
  }
}

// --------------------------------------------------------------------------------
gboolean TableSet::StartClassification (GNode    *node,
                                        TableSet *table_set)
{
  NodeData *data = (NodeData *) node->data;

  if (data->_match)
  {
    Player *winner = data->_match->GetWinner ();

    if (winner && g_slist_find (table_set->_result_list,
                                winner) == NULL)
    {
      table_set->_result_list = g_slist_append (table_set->_result_list,
                                                winner);
      winner->SetData (table_set,
                       "best_table",
                       (void *) data->_table);
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean TableSet::CloseClassification (GNode    *node,
                                        TableSet *table_set)
{
  NodeData *data = (NodeData *) node->data;

  if (data->_match)
  {
    Player *winner = data->_match->GetWinner ();

    if (winner)
    {
      winner->RemoveData (table_set,
                          "best_table");
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
void TableSet::OnAttrListUpdated ()
{
  Display ();
}

// --------------------------------------------------------------------------------
void TableSet::OnFromTableComboboxChanged ()
{
  guint nb_table_to_display = _nb_tables - gtk_combo_box_get_active (GTK_COMBO_BOX (_glade->GetWidget ("from_table_combobox")));

  for (guint t = 0; t < _nb_tables; t++)
  {
    if (t < nb_table_to_display)
    {
      _tables[t]->Show (nb_table_to_display - t);
    }
    else
    {
      _tables[t]->Hide ();
    }
  }

  Display ();
}

// --------------------------------------------------------------------------------
gboolean TableSet::Stuff (GNode    *node,
                          TableSet *table_set)
{
  NodeData *data = (NodeData *) node->data;

  if (   (data->_table == table_set->_tables[table_set->_table_to_stuff])
      && (data->_match))
  {
    Player *A      = data->_match->GetPlayerA ();
    Player *B      = data->_match->GetPlayerB ();
    Player *winner;

    if (A && B)
    {
      if (g_random_boolean ())
      {
        data->_match->SetScore (A, table_set->_max_score->_value, TRUE);
        data->_match->SetScore (B, g_random_int_range (0,
                                                       table_set->_max_score->_value), FALSE);
        winner = A;
      }
      else
      {
        data->_match->SetScore (A, g_random_int_range (0,
                                                       table_set->_max_score->_value), FALSE);
        data->_match->SetScore (B, table_set->_max_score->_value, TRUE);
        winner = B;
      }
    }
    else
    {
      winner = data->_match->GetWinner ();
    }

    if (winner)
    {
      GNode *parent = node->parent;

      if (parent)
      {
        NodeData *parent_data = (NodeData *) parent->data;

        table_set->SetPlayerToMatch (parent_data->_match,
                                     winner,
                                     g_node_child_position (parent, node));
      }
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
Player *TableSet::GetPlayerFromRef (guint ref)
{
  return _supervisor->GetPlayerFromRef (ref);
}

// --------------------------------------------------------------------------------
void TableSet::SetPlayerToMatch (Match  *to_match,
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

    path = (GtkTreePath *) to_match->GetPtrData (this,
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

        gtk_tree_store_set (_quick_search_treestore, &iter,
                            QUICK_MATCH_PATH_COLUMN, to_match->GetName (),
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
void TableSet::OnStuffClicked ()
{
  if (_tree_root)
  {
    for (_table_to_stuff = _nb_tables-1; _table_to_stuff >= 0; _table_to_stuff--)
    {
      g_node_traverse (_tree_root,
                       G_POST_ORDER,
                       G_TRAVERSE_ALL,
                       -1,
                       (GNodeTraverseFunc) Stuff,
                       this);
    }

    RefreshTableStatus ();

    for (guint t = 1; t < _nb_tables; t++)
    {
      Table *table = _tables[t];

      if (table->_is_over)
      {
        _supervisor->OnTableOver (this,
                                  table);
      }
    }
  }
}

// --------------------------------------------------------------------------------
void TableSet::OnInputToggled (GtkWidget *widget)
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
void TableSet::OnMatchSheetToggled (GtkWidget *widget)
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
void TableSet::OnDisplayToggled (GtkWidget *widget)
{
  GtkWidget *vbox = _glade->GetWidget ("display_vbox");

  if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (widget)))
  {
    gtk_widget_show (vbox);
  }
  else
  {
    gtk_widget_hide (vbox);
  }
}

// --------------------------------------------------------------------------------
GSList *TableSet::GetCurrentClassification ()
{
  _result_list = NULL;

  if (_tree_root)
  {
    g_node_traverse (_tree_root,
                     G_LEVEL_ORDER,
                     G_TRAVERSE_ALL,
                     -1,
                     (GNodeTraverseFunc) StartClassification,
                     this);

    _result_list = g_slist_sort_with_data (_result_list,
                                           (GCompareDataFunc) ComparePlayer,
                                           (void *) this);

    {
      Player::AttributeId *attr_id = new Player::AttributeId ("rank", GetDataOwner ());
      GSList              *current;
      guint                previous_rank   = 0;
      Player              *previous_player = NULL;
      guint32              rand_seed = _rand_seed;

      _rand_seed = 0; // !!
      current = _result_list;
      for (guint i = _first_place; current != NULL; i++)
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

    g_node_traverse (_tree_root,
                     G_LEVEL_ORDER,
                     G_TRAVERSE_ALL,
                     -1,
                     (GNodeTraverseFunc) CloseClassification,
                     this);
  }

  return _result_list;
}

// --------------------------------------------------------------------------------
guint TableSet::GetFirstPlace ()
{
  return _first_place;
}

// --------------------------------------------------------------------------------
gint TableSet::ComparePlayer (Player   *A,
                              Player   *B,
                              TableSet *table_set)
{
  if (B == NULL)
  {
    return 1;
  }
  else if (A == NULL)
  {
    return -1;
  }

  {
    Player::AttributeId attr_id ("status", table_set->GetDataOwner ());

    if (A->GetAttribute (&attr_id) && B->GetAttribute (&attr_id))
    {
      gchar *status_A = A->GetAttribute (&attr_id)->GetStrValue ();
      gchar *status_B = B->GetAttribute (&attr_id)->GetStrValue ();

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
    Table *table_A = (Table *) A->GetPtrData (table_set, "best_table");
    Table *table_B = (Table *) B->GetPtrData (table_set, "best_table");

    if (table_A != table_B)
    {
      return table_B->GetColumn () - table_A->GetColumn ();
    }
  }

  return table_set->ComparePreviousRankPlayer (A,
                                               B,
                                               table_set->_rand_seed);
}

// --------------------------------------------------------------------------------
gint TableSet::ComparePreviousRankPlayer (Player  *A,
                                          Player  *B,
                                          guint32  rand_seed)
{
  Stage               *previous = _supervisor->GetPreviousStage ();
  Player::AttributeId  attr_id ("rank", previous);

  attr_id.MakeRandomReady (rand_seed);
  return Player::Compare (A,
                          B,
                          &attr_id);
}

// --------------------------------------------------------------------------------
void TableSet::OnSearchMatch ()
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
void TableSet::OnPrinterSettingClicked (GtkWidget *widget)
{
  GtkPrintOperation *operation      = (GtkPrintOperation *) g_object_get_data (G_OBJECT (_preview), "preview_operation");
  GtkPrintSettings  *print_settings = gtk_print_operation_get_print_settings (operation);
  GtkPageSetup      *page_setup;
  GtkPageSetup      *new_page_setup;


  page_setup = gtk_page_setup_new_from_key_file (_config_file,
                                                 "page_setup",
                                                 NULL);

  new_page_setup = gtk_print_run_page_setup_dialog (GTK_WINDOW (gtk_widget_get_toplevel (widget)),
                                                    page_setup,
                                                    print_settings);

  if (new_page_setup)
  {
    gtk_page_setup_to_key_file (new_page_setup,
                                _config_file,
                                "page_setup");
  }

  if (page_setup)
  {
    g_object_unref (page_setup);
  }
  page_setup = new_page_setup;
}

// --------------------------------------------------------------------------------
void TableSet::OnBeginPrint (GtkPrintOperation *operation,
                             GtkPrintContext   *context)
{
  gdouble paper_w = gtk_print_context_get_width  (context);
  gdouble paper_h = gtk_print_context_get_height (context);

  if (_print_full_table)
  {
    gdouble canvas_x;
    gdouble canvas_y;
    gdouble canvas_w;
    gdouble canvas_h;
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

    if (paper_w > paper_h)
    {
      _nb_match_per_sheet = 2;
    }
    else
    {
      _nb_match_per_sheet = 4;
    }

    nb_match = g_slist_length (_match_to_print);
    nb_page  = nb_match/_nb_match_per_sheet;

    if (nb_match%_nb_match_per_sheet != 0)
    {
      nb_page++;
    }

    gtk_print_operation_set_n_pages (operation,
                                     nb_page);
  }
}

// --------------------------------------------------------------------------------
gboolean TableSet::on_preview_expose (GtkWidget      *drawing_area,
                                      GdkEventExpose *event,
                                      TableSet       *ts)
{
  guint page_number = (guint) g_object_get_data (G_OBJECT (drawing_area), "page_number");

  gdk_window_clear (drawing_area->window);

  ts->_current_preview_area = drawing_area;
  gtk_print_operation_preview_render_page (ts->_preview,
                                           page_number-1);

  return TRUE;
}

// --------------------------------------------------------------------------------
void TableSet::OnPreviewClicked (GtkWidget *widget)
{
    GtkWidget *w = _glade->GetWidget ("table_radiobutton");

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
    {
      _print_full_table = TRUE;
    }
    else
    {
      GtkWidget *w         = _glade->GetWidget ("all_radiobutton");
      gboolean   all_sheet = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));

      LookForMatchToPrint (NULL,
                           all_sheet);
      _print_full_table = FALSE;
    }
    Print (NULL);
}

// --------------------------------------------------------------------------------
gboolean TableSet::OnPreview (GtkPrintOperation        *operation,
                              GtkPrintOperationPreview *preview,
                              GtkPrintContext          *context,
                              GtkWindow                *parent)
{
  _preview = preview;

  g_object_set_data (G_OBJECT (preview), "preview_context",   context);
  g_object_set_data (G_OBJECT (preview), "preview_operation", operation);

  {
    cairo_t *cr = goo_canvas_create_cairo_context (GetCanvas ());
    gdouble  canvas_dpi;

    g_object_get (G_OBJECT (GetCanvas ()),
                  "resolution-x", &canvas_dpi,
                  NULL);

    gtk_print_context_set_cairo_context (context,
                                         cr,
                                         canvas_dpi,
                                         canvas_dpi);
    cairo_destroy (cr);
  }

  gtk_widget_hide (_print_dialog);

  return TRUE;
}

// --------------------------------------------------------------------------------
void TableSet::OnPreviewReady (GtkPrintOperationPreview *preview,
                               GtkPrintContext          *context)
{
  {
    gdouble    paper_w        = gtk_print_context_get_width  (context);
    gdouble    paper_h        = gtk_print_context_get_height (context);
    guint      spacing        = 5;
    guint      drawing_w      = 200;
    guint      drawing_h      = drawing_w*paper_h/paper_w;
    GtkWidget *preview_layout = _glade->GetWidget ("preview_layout");

    g_object_set (G_OBJECT (preview_layout),
                  "width",  _print_nb_x_pages*(drawing_w+spacing),
                  "height", _print_nb_y_pages*(drawing_h+spacing),
                  NULL);

    for (guint x = 0; x < _print_nb_x_pages; x++)
    {
      for (guint y = 0; y < _print_nb_y_pages; y++)
      {
        GtkWidget *drawing_area = gtk_drawing_area_new ();
        GdkColor   bg_color;

        gdk_color_parse ("#FFFFFF", &bg_color);
        gtk_widget_modify_bg (drawing_area,
                              GTK_STATE_NORMAL,
                              &bg_color);

        gtk_widget_set_size_request (GTK_WIDGET (drawing_area),
                                     drawing_w,
                                     drawing_h);

        gtk_layout_put (GTK_LAYOUT (preview_layout),
                        drawing_area,
                        x*(drawing_w+spacing),
                        y*(drawing_h+spacing));

        g_object_set_data (G_OBJECT (drawing_area), "page_number", (void *) (y*_print_nb_x_pages + (x+1)));
        g_signal_connect (drawing_area, "expose_event",
                          G_CALLBACK (on_preview_expose),
                          this);

        gtk_widget_show_all (drawing_area);
      }
    }
  }

  if (gtk_dialog_run (GTK_DIALOG (_preview_dialog)) == GTK_RESPONSE_OK)
  {
    Print ("Table");
  }

#if 0
  {
    GtkWidget *view_port  = _glade->GetWidget ("preview_viewport");
    GtkWidget *canvas     = gtk_bin_get_child (GTK_BIN (view_port));

    gtk_widget_destroy (canvas);
  }
#endif

  gtk_print_operation_preview_end_preview (preview);
  gtk_widget_hide (_preview_dialog);
}

// --------------------------------------------------------------------------------
void TableSet::OnPreviewGotPageSize (GtkPrintOperationPreview *preview,
                                     GtkPrintContext          *context,
                                     GtkPageSetup             *page_setup)
{
  cairo_t      *cr         = gdk_cairo_create (_current_preview_area->window);
  GtkPaperSize *paper_size = gtk_page_setup_get_paper_size (page_setup);
  gdouble       paper_w    = gtk_paper_size_get_width (paper_size, GTK_UNIT_POINTS);
  gdouble       canvas_dpi;
  gdouble       drawing_dpi;

  g_object_get (G_OBJECT (GetCanvas ()),
                "resolution-x", &canvas_dpi,
                NULL);

  paper_w = paper_w * canvas_dpi / 72.0;

  drawing_dpi = 200.0 * canvas_dpi / paper_w;

  _print_scale = drawing_dpi/canvas_dpi;

  gtk_print_context_set_cairo_context (context,
                                       cr,
                                       drawing_dpi,
                                       drawing_dpi);
  cairo_destroy (cr);
}

// --------------------------------------------------------------------------------
void TableSet::OnDrawPage (GtkPrintOperation *operation,
                           GtkPrintContext   *context,
                           gint               page_nr)
{
  gdouble paper_w   = gtk_print_context_get_width  (context);
  gdouble paper_h   = gtk_print_context_get_height (context);
  Module *container = dynamic_cast <Module *> (_supervisor);

  if (_print_full_table)
  {
    cairo_t         *cr       = gtk_print_context_get_cairo_context (context);
    guint            x_page   = page_nr % _print_nb_x_pages;
    guint            y_page   = page_nr / _print_nb_x_pages;
    gdouble          header_h = (PRINT_HEADER_HEIGHT+2) * paper_w  / 100;
    GooCanvasBounds  bounds;

    if (y_page == 0)
    {
      container->DrawContainerPage (operation,
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
                                 _nb_match_per_sheet*page_nr);
    for (guint i = 0; current_match && (i < _nb_match_per_sheet); i++)
    {
      GooCanvasItem *match_group;

      match_group = goo_canvas_group_new (goo_canvas_get_root_item (canvas),
                                          NULL);

      {
        cairo_matrix_t matrix;

        cairo_matrix_init_translate (&matrix,
                                     0.0,
                                     i*(100.0/_nb_match_per_sheet) * paper_h/paper_w);

        g_object_set_data (G_OBJECT (operation), "operation_matrix", (void *) &matrix);

        container->DrawContainerPage (operation,
                                      context,
                                      page_nr);
      }

      {
        Match         *match  = (Match *) current_match->data;
        Player        *A      = match->GetPlayerA ();
        Player        *B      = match->GetPlayerB ();
        GooCanvasItem *name_item;
        GooCanvasItem *title_group = goo_canvas_group_new (match_group, NULL);
        gchar         *font        = g_strdup_printf ("Sans Bold %fpx", 3.5/2.0*(PRINT_FONT_HEIGHT));

        {
          gchar *decimal = strchr (font, ',');

          if (decimal)
          {
            *decimal = '.';
          }
        }

        match->SetData (this, "printed", (void *) TRUE);

        // Match ID
        {
          gdouble  offset       = i * ((100.0/_nb_match_per_sheet) * paper_h/paper_w) + (PRINT_HEADER_HEIGHT + 10.0);
          gchar   *match_number = g_strdup_printf (gettext ("Match %s"), match->GetName ());

          name_item = goo_canvas_text_new (title_group,
                                           match_number,
                                           0.0,
                                           offset - 6.0,
                                           -1.0,
                                           GTK_ANCHOR_W,
                                           "fill-color", "grey",
                                           "font", font,
                                           NULL);
          g_free (match_number);
        }

        // Referee / Track
        {
          GooCanvasItem *referee_group = goo_canvas_group_new (title_group, NULL);
          GooCanvasItem *track_group   = goo_canvas_group_new (title_group, NULL);

          goo_canvas_rect_new (referee_group,
                               30.0,
                               0.0,
                               35.0,
                               6.0,
                               "stroke-color", "Grey95",
                               "line-width", 0.2,
                               NULL);
          goo_canvas_text_new (referee_group,
                               gettext ("Referee"),
                               30.0,
                               0.0,
                               -1,
                               GTK_ANCHOR_W,
                               "fill-color", "Black",
                               "font", font,
                               NULL);

          Canvas::Align (referee_group,
                         NULL,
                         name_item);

          goo_canvas_rect_new (track_group,
                               0.0,
                               0.0,
                               35.0,
                               6.0,
                               "stroke-color", "Grey95",
                               "line-width", 0.2,
                               NULL);
          goo_canvas_text_new (track_group,
                               gettext ("Piste"),
                               0.0,
                               0.0,
                               -1,
                               GTK_ANCHOR_W,
                               "fill-color", "Black",
                               "font", font,
                               NULL);

          Canvas::Align (track_group,
                         NULL,
                         referee_group);
          Canvas::Anchor (track_group,
                          NULL,
                          referee_group,
                          20);
        }

        // Matchs
        {
          GooCanvasItem *match_table = goo_canvas_table_new (match_group,
                                                             "column-spacing", 1.0,
                                                             "row-spacing",    2.0,
                                                             NULL);

          DrawPlayerMatch (match_table,
                           match,
                           A,
                           0);
          DrawPlayerMatch (match_table,
                           match,
                           B,
                           1);

          Canvas::Anchor (match_table,
                          title_group,
                          NULL,
                          80);
          Canvas::Align (match_table,
                         title_group,
                         NULL);
        }

        g_free (font);
      }

      Canvas::FitToContext (match_group,
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
void TableSet::DrawPlayerMatch (GooCanvasItem *table,
                                Match         *match,
                                Player        *player,
                                guint          row)
{
  GooCanvasItem *text_item;
  GooCanvasItem *goo_rect;

  // Name
  {
    GString *image;
    gchar   *font = g_strdup_printf ("Sans Bold %fpx", PRINT_FONT_HEIGHT);
    gchar   *decimal = strchr (font, ',');

    if (decimal)
    {
      *decimal = '.';
    }

    image     = GetPlayerImage (player);
    text_item = Canvas::PutTextInTable (table,
                                        image->str,
                                        row,
                                        0);
    g_string_free (image,
                   TRUE);
    Canvas::SetTableItemAttribute (text_item, "y-align", 0.5);

    g_object_set (G_OBJECT (text_item),
                  "font", font,
                  "ellipsize", PANGO_ELLIPSIZE_NONE,
                  "anchor", GTK_ANCHOR_WEST,
                  NULL);
    g_free (font);
  }

  // Score
  {
    GooCanvasItem *score_table = match->GetScoreTable (table,
                                                       PRINT_FONT_HEIGHT);

    Canvas::PutInTable (table,
                        score_table,
                        row,
                        1);
    Canvas::SetTableItemAttribute (score_table, "y-align", 0.5);
  }

  {
    gdouble score_size = 4.0;

    goo_rect = goo_canvas_rect_new (table,
                                    0.0,
                                    0.0,
                                    score_size,
                                    score_size,
                                    "line-width", 0.3,
                                    NULL);
    Canvas::PutInTable (table,
                        goo_rect,
                        row,
                        2);
    Canvas::SetTableItemAttribute (goo_rect, "y-align", 0.5);

    goo_rect = goo_canvas_rect_new (table,
                                    0.0,
                                    0.0,
                                    score_size*5,
                                    score_size,
                                    "fill-color", "Grey85",
                                    "line-width", 0.0,
                                    NULL);
    Canvas::PutInTable (table,
                        goo_rect,
                        row,
                        3);
    Canvas::SetTableItemAttribute (goo_rect, "y-align", 0.5);

    text_item = Canvas::PutTextInTable (table,
                                        gettext ("Signature"),
                                        row,
                                        3);
    Canvas::SetTableItemAttribute (text_item, "y-align", 0.5);
    g_object_set (G_OBJECT (text_item),
                  "font", "Sans bold 3.5px",
                  "fill-color", "Grey95",
                  NULL);
  }
}

// --------------------------------------------------------------------------------
void TableSet::OnZoom (gdouble value)
{
  goo_canvas_set_scale (GetCanvas (),
                        value);
}

// --------------------------------------------------------------------------------
void TableSet::OnPlugged ()
{
  CanvasModule::OnPlugged ();
  gtk_container_add (GTK_CONTAINER (_control_container),
                     _from_widget);
}

// --------------------------------------------------------------------------------
void TableSet::OnUnPlugged ()
{
  if (gtk_widget_get_parent (_from_widget))
  {
    gtk_container_remove (GTK_CONTAINER (_control_container),
                          _from_widget);
  }
  CanvasModule::OnUnPlugged ();
}

// --------------------------------------------------------------------------------
void TableSet::OnPrint ()
{
  if (gtk_dialog_run (GTK_DIALOG (_print_dialog)) == GTK_RESPONSE_OK)
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

      LookForMatchToPrint (NULL,
                           all_sheet);
      _print_full_table = FALSE;
      Print (gettext ("Score sheet"));
    }
  }

  gtk_widget_hide (_print_dialog);
}

// --------------------------------------------------------------------------------
gboolean TableSet::OnPrintTable (GooCanvasItem  *item,
                                 GooCanvasItem  *target_item,
                                 GdkEventButton *event,
                                 TableSet       *table_set)
{
  Table *table_to_print = (Table *) g_object_get_data (G_OBJECT (item), "table_to_print");
  gchar *title          = g_strdup_printf (gettext ("%s: score sheets printing"), table_to_print->GetImage ());

  gtk_window_set_title (GTK_WINDOW (table_set->_table_print_dialog),
                        title);
  g_free (title);

  if (gtk_dialog_run (GTK_DIALOG (table_set->_table_print_dialog)) == GTK_RESPONSE_OK)
  {
    GtkWidget *w         = table_set->_glade->GetWidget ("table_all_radiobutton");
    gboolean   all_sheet = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));

    table_set->LookForMatchToPrint (table_to_print,
                                    all_sheet);
    table_set->_print_full_table = FALSE;
    table_set->Print (gettext ("Score sheet"));
  }

  gtk_widget_hide (table_set->_table_print_dialog);

  return TRUE;
}

// --------------------------------------------------------------------------------
gboolean TableSet::OnPrintMatch (GooCanvasItem  *item,
                                 GooCanvasItem  *target_item,
                                 GdkEventButton *event,
                                 TableSet       *table_set)
{
  if (table_set->_match_to_print)
  {
    g_slist_free (table_set->_match_to_print);
    table_set->_match_to_print = NULL;
  }

  table_set->_match_to_print = g_slist_prepend (table_set->_match_to_print,
                                                g_object_get_data (G_OBJECT (item), "match_to_print"));
  table_set->_print_full_table = FALSE;
  table_set->Print (gettext ("Score sheet"));

  return TRUE;
}

// --------------------------------------------------------------------------------
void TableSet::OnStatusChanged (GtkComboBox *combo_box)
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
gboolean TableSet::on_status_arrow_press (GooCanvasItem  *item,
                                          GooCanvasItem  *target,
                                          GdkEventButton *event,
                                          TableSet       *table_set)
{
  if (   (event->button == 1)
      && (event->type   == GDK_BUTTON_PRESS))
  {
    GtkWidget *combo;
    NodeData  *data;
    Player    *winner;
    NodeData  *parent_data = NULL;

    {
      GNode *node = (GNode *) g_object_get_data (G_OBJECT (item), "TableSet::node");

      if (node->parent)
      {
        parent_data = (NodeData *) node->parent->data;
      }
      data = (NodeData *) node->data;
    }

    winner = data->_match->GetWinner ();

    {
      GtkCellRenderer *cell = gtk_cell_renderer_pixbuf_new ();

      combo = gtk_combo_box_new_with_model (table_set->GetStatusModel ());

      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, FALSE);
      gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo),
                                      cell, "pixbuf", AttributeDesc::DISCRETE_ICON,
                                      NULL);

      goo_canvas_widget_new (table_set->GetRootItem (),
                             combo,
                             event->x_root,
                             event->y_root,
                             _score_rect_size*3.3/2.0,
                             _score_rect_size,
                             NULL);
      gtk_widget_grab_focus (combo);
    }

    {
      Player::AttributeId  attr_id ("status", table_set->GetDataOwner ());
      Attribute           *attr   = winner->GetAttribute (&attr_id);

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
          text = attr->GetStrValue ();
        }

        iter_is_valid = gtk_tree_model_get_iter_first (table_set->GetStatusModel (),
                                                       &iter);
        for (guint i = 0; iter_is_valid; i++)
        {
          gtk_tree_model_get (table_set->GetStatusModel (),
                              &iter,
                              AttributeDesc::DISCRETE_XML_IMAGE, &code,
                              -1);
          if (strcmp (text, code) == 0)
          {
            gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo),
                                           &iter);

            break;
          }
          iter_is_valid = gtk_tree_model_iter_next (table_set->GetStatusModel (),
                                                    &iter);
        }
      }
    }

    if (parent_data)
    {
      g_object_set_data (G_OBJECT (combo), "player_for_status", (void *) winner);
      g_object_set_data (G_OBJECT (combo), "match_for_status", (void *) parent_data->_match);
      g_signal_connect (combo, "changed",
                        G_CALLBACK (on_status_changed), table_set);
      g_signal_connect (combo, "key-press-event",
                        G_CALLBACK (on_status_key_press_event), table_set);
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
void TableSet::on_status_changed (GtkComboBox *combo_box,
                                  TableSet    *table_set)
{
  table_set->OnStatusChanged (combo_box);
  gtk_widget_destroy (GTK_WIDGET (combo_box));
}

// --------------------------------------------------------------------------------
gboolean TableSet::on_status_key_press_event (GtkWidget   *widget,
                                              GdkEventKey *event,
                                              gpointer     user_data)
{
  if (event->keyval == GDK_Escape)
  {
    gtk_widget_destroy (GTK_WIDGET (widget));
    return TRUE;
  }
  return FALSE;
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_match_sheet_radiobutton_toggled (GtkWidget *widget,
                                                                    Object    *owner)
{
  TableSet *t = dynamic_cast <TableSet *> (owner);

  t->OnMatchSheetToggled (widget);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_from_table_combobox_changed (GtkWidget *widget,
                                                                Object    *owner)
{
  TableSet *t = dynamic_cast <TableSet *> (owner);

  t->OnFromTableComboboxChanged ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_quick_search_combobox_changed (GtkWidget *widget,
                                                                  Object    *owner)
{
  TableSet *t = dynamic_cast <TableSet *> (owner);

  t->OnSearchMatch ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_printer_setting_button_clicked (GtkWidget *widget,
                                                                   Object    *owner)
{
  TableSet *t = dynamic_cast <TableSet *> (owner);

  t->OnPrinterSettingClicked (widget);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_preview_button_clicked (GtkWidget *widget,
                                                           Object    *owner)
{
  TableSet *t = dynamic_cast <TableSet *> (owner);

  t->OnPreviewClicked (widget);
}
