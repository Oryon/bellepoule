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
#include "table_set.hpp"
#include "table_supervisor.hpp"

const gchar *TableSupervisor::_class_name     = N_("Table");
const gchar *TableSupervisor::_xml_class_name = "PhaseDeTableaux";

typedef enum
{
  FROM_NAME_COLUMN,
  FROM_STATUS_COLUMN
} FromColumnId;

typedef enum
{
  TABLE_SET_NAME_COLUMN,
  TABLE_SET_TABLE_COLUMN,
  TABLE_SET_STATUS_COLUMN,
  TABLE_SET_VISIBILITY_COLUMN
} DisplayColumnId;

extern "C" G_MODULE_EXPORT void on_table_qualified_ratio_spinbutton_value_changed (GtkSpinButton *spinbutton,
                                                                                   Object        *owner);

// --------------------------------------------------------------------------------
TableSupervisor::TableSupervisor (StageClass *stage_class)
  : Stage (stage_class),
  Module ("table_supervisor.glade")
{
  _max_score = new Data ("ScoreMax",
                         10);

  _fenced_places = new Data ("PlacesTirees",
                             (guint) NONE);

  _displayed_table_set = NULL;

  {
    AddSensitiveWidget (_glade->GetWidget ("input_toolbutton"));
    AddSensitiveWidget (_glade->GetWidget ("qualified_ratio_spinbutton"));
    AddSensitiveWidget (_glade->GetWidget ("nb_qualified_spinbutton"));
    AddSensitiveWidget (_glade->GetWidget ("classification_vbox"));
    AddSensitiveWidget (_glade->GetWidget ("max_score_entry"));
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
#ifndef DEBUG
    filter->ShowAttribute ("first_name");
    filter->ShowAttribute ("club");
#endif

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

  _table_set_treestore = GTK_TREE_STORE (_glade->GetObject ("table_set_treestore"));
  _table_set_filter    = GTK_TREE_MODEL_FILTER (_glade->GetObject ("table_set_treemodelfilter"));

  gtk_tree_model_filter_set_visible_column (_table_set_filter,
                                            TABLE_SET_VISIBILITY_COLUMN);
}

// --------------------------------------------------------------------------------
TableSupervisor::~TableSupervisor ()
{
  _max_score->Release ();

  DeleteTableSets ();
}

// --------------------------------------------------------------------------------
void TableSupervisor::Init ()
{
  RegisterStageClass (gettext (_class_name),
                      _xml_class_name,
                      CreateInstance,
                      EDITABLE);
}

// --------------------------------------------------------------------------------
Stage *TableSupervisor::CreateInstance (StageClass *stage_class)
{
  return new TableSupervisor (stage_class);
}

// --------------------------------------------------------------------------------
void TableSupervisor::Display ()
{
  Wipe ();

  OnTableSetSelected (_displayed_table_set);
}

// --------------------------------------------------------------------------------
void TableSupervisor::OnTableSetSelected (TableSet *table_set)
{
  if (_displayed_table_set)
  {
    _displayed_table_set->UnPlug ();
    _displayed_table_set = NULL;
  }

  if (table_set)
  {
    Plug (table_set,
          GetWidget ("table_set_hook"));
    table_set->Display ();
    table_set->RestoreZoomFactor (GTK_SCALE (_glade->GetWidget ("zoom_scale")));

    _displayed_table_set = table_set;
  }
}

// --------------------------------------------------------------------------------
gboolean TableSupervisor::IsOver ()
{
  _is_over = TRUE;

  gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_filter),
                          (GtkTreeModelForeachFunc) TableSetIsOver,
                          this);

  return _is_over;
}

// --------------------------------------------------------------------------------
gboolean TableSupervisor::TableSetIsOver (GtkTreeModel    *model,
                                          GtkTreePath     *path,
                                          GtkTreeIter     *iter,
                                          TableSupervisor *ts)
{
  TableSet *table_set;

  gtk_tree_model_get (model, iter,
                      TABLE_SET_TABLE_COLUMN, &table_set,
                      -1);
  ts->_is_over &= table_set->IsOver ();

  return FALSE;
}

// --------------------------------------------------------------------------------
void TableSupervisor::CreateTableSets ()
{
  if (_attendees)
  {
    guint nb_players = g_slist_length (_attendees->GetShortList ());
    guint nb_tables  = 0;

    for (guint i = 0; i < 32; i++)
    {
      guint bit_cursor;

      bit_cursor = 1;
      bit_cursor = bit_cursor << i;
      if (bit_cursor >= nb_players)
      {
        nb_tables = i++;
        break;
      }
    }

    if (nb_tables > 0)
    {
      {
        gtk_tree_store_clear (_table_set_treestore);

        FeedTableSetStore (1,
                           nb_tables,
                           NULL);
      }

      {
        GtkTreeIter  iter;
        TableSet    *first_table_set;

        gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_table_set_treestore),
                                       &iter);
        gtk_tree_model_get (GTK_TREE_MODEL (_table_set_treestore), &iter,
                            TABLE_SET_TABLE_COLUMN, &first_table_set,
                            -1);

        if (first_table_set)
        {
          first_table_set->SetAttendees (g_slist_copy (_attendees->GetShortList ()));
        }
      }
    }
  }

  SetTableSetsState ();
}

// --------------------------------------------------------------------------------
void TableSupervisor::SetTableSetsState ()
{
  gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_treestore),
                          (GtkTreeModelForeachFunc) ActivateTableSet,
                          this);
  gtk_tree_view_expand_all (GTK_TREE_VIEW (_glade->GetWidget ("table_set_treeview")));

  if (_displayed_table_set == NULL)
  {
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_table_set_treestore),
                                       &iter))
    {
      GtkTreePath *path;

      path = gtk_tree_model_get_path (GTK_TREE_MODEL (_table_set_treestore), &iter);

      gtk_tree_view_set_cursor (GTK_TREE_VIEW (_glade->GetWidget ("table_set_treeview")),
                                path,
                                NULL,
                                FALSE);

      gtk_tree_path_free (path);
    }
  }

  SignalStatusUpdate ();
}

// --------------------------------------------------------------------------------
gboolean TableSupervisor::ActivateTableSet (GtkTreeModel    *model,
                                            GtkTreePath     *path,
                                            GtkTreeIter     *iter,
                                            TableSupervisor *ts)
{
  TableSet *table_set;
  gboolean  visibility = FALSE;

  gtk_tree_model_get (model, iter,
                      TABLE_SET_TABLE_COLUMN, &table_set,
                      -1);

  if (table_set->GetFirstPlace () == 1)
  {
    visibility = TRUE;
  }
  else if (ts->_fenced_places->_value == ALL_PLACES)
  {
    visibility = TRUE;
  }
  else if (ts->_fenced_places->_value == THIRD_PLACES)
  {
    if (table_set->GetFirstPlace () <= 3)
    {
      visibility = TRUE;
    }
  }
  else if (ts->_fenced_places->_value == QUOTA)
  {
  }

  gtk_tree_store_set (ts->_table_set_treestore, iter,
                      TABLE_SET_VISIBILITY_COLUMN, visibility,
                      -1);

  if (visibility)
  {
    table_set->Activate ();
  }
  else
  {
    table_set->DeActivate ();
  }

  if (ts->_displayed_table_set && (visibility == FALSE))
  {
    if (ts->_displayed_table_set == table_set)
    {
      ts->_displayed_table_set->UnPlug ();
      ts->_displayed_table_set = NULL;
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
TableSet *TableSupervisor::GetTableSet (gchar *id)
{
  GtkTreeIter  iter;
  GtkTreePath *path = gtk_tree_path_new_from_string (id);
  TableSet    *table_set = NULL;

  if (gtk_tree_model_get_iter (GTK_TREE_MODEL (_table_set_treestore),
                               &iter,
                               path))
  {
    gtk_tree_model_get (GTK_TREE_MODEL (_table_set_treestore), &iter,
                        TABLE_SET_TABLE_COLUMN, &table_set,
                        -1);
  }

  gtk_tree_path_free (path);

  return table_set;
}

// --------------------------------------------------------------------------------
gboolean TableSupervisor::DeleteTableSet (GtkTreeModel *model,
                                          GtkTreePath  *path,
                                          GtkTreeIter  *iter,
                                          gpointer      data)
{
  TableSet *table_set;

  gtk_tree_model_get (model, iter,
                      TABLE_SET_TABLE_COLUMN, &table_set,
                      -1);
  if (table_set)
  {
    table_set->Release ();
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
void TableSupervisor::DeleteTableSets ()
{
  if (_displayed_table_set)
  {
    _displayed_table_set->UnPlug ();
    _displayed_table_set = NULL;
  }

  gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_treestore),
                          DeleteTableSet,
                          NULL);
  gtk_tree_store_clear (_table_set_treestore);
}

// --------------------------------------------------------------------------------
void TableSupervisor::Garnish ()
{
  DeleteTableSets ();
  CreateTableSets ();
}

// --------------------------------------------------------------------------------
void TableSupervisor::LoadConfiguration (xmlNode *xml_node)
{
  Stage::LoadConfiguration (xml_node);

  if (_fenced_places)
  {
    _fenced_places->Load (xml_node);
  }
}

// --------------------------------------------------------------------------------
void TableSupervisor::Load (xmlNode *xml_node)
{
  LoadConfiguration (xml_node);

  for (xmlNode *n = xml_node; n != NULL; n = n->next)
  {
    if (n->type == XML_ELEMENT_NODE)
    {
      if (strcmp ((char *) n->name, _xml_class_name) == 0)
      {
      }
      else if (strcmp ((char *) n->name, "Tireur") == 0)
      {
        LoadAttendees (n);
      }
      else if (strcmp ((char *) n->name, "SuiteDeTableaux") == 0)
      {
        GtkTreeIter  iter;
        TableSet    *table_set;
        gchar       *prop = (gchar *) xmlGetProp (n, BAD_CAST "ID");

        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_table_set_treestore),
                                           &iter) == FALSE)
        {
          CreateTableSets ();
        }

        if (prop)
        {
          table_set = GetTableSet (prop);
          if (table_set)
          {
            table_set->Load (n);
          }
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
void TableSupervisor::SaveConfiguration (xmlTextWriter *xml_writer)
{
  Stage::SaveConfiguration (xml_writer);

  if (_fenced_places)
  {
    _fenced_places->Save (xml_writer);
  }
}

// --------------------------------------------------------------------------------
void TableSupervisor::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST _xml_class_name);

  SaveConfiguration (xml_writer);
  SaveAttendees     (xml_writer);

  gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_filter),
                          (GtkTreeModelForeachFunc) SaveTableSet,
                          xml_writer);

  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
gboolean TableSupervisor::SaveTableSet (GtkTreeModel  *model,
                                        GtkTreePath   *path,
                                        GtkTreeIter   *iter,
                                        xmlTextWriter *xml_writer)
{
  TableSet *table_set;

  gtk_tree_model_get (model, iter,
                      TABLE_SET_TABLE_COLUMN, &table_set,
                      -1);
  table_set->Save (xml_writer);

  return FALSE;
}

// --------------------------------------------------------------------------------
void TableSupervisor::FeedTableSetStore (guint        from_place,
                                         guint        nb_tables,
                                         GtkTreeIter *parent)
{
  GtkTreeIter  iter;
  TableSet    *table_set;

  gtk_tree_store_append (_table_set_treestore,
                         &iter,
                         parent);

  {
    GtkTreePath *path  = gtk_tree_model_get_path (GTK_TREE_MODEL (_table_set_treestore), &iter);

    table_set = new TableSet (this,
                              gtk_tree_path_to_string (path),
                              _glade->GetWidget ("from_vbox"),
                              from_place);

    {
      GtkTreeRowReference *ref = gtk_tree_row_reference_new (GTK_TREE_MODEL (_table_set_treestore),
                                                             path);
      table_set->SetData (this, "tree_row_ref",
                          ref,
                          (GDestroyNotify) gtk_tree_row_reference_free);
    }

    gtk_tree_path_free (path);

    table_set->SetDataOwner (this);
    table_set->SetStatusCbk ((TableSet::StatusCbk) OnTableSetStatusUpdated,
                             this);

    gtk_tree_store_set (_table_set_treestore, &iter,
                        TABLE_SET_NAME_COLUMN, table_set->GetName (),
                        TABLE_SET_TABLE_COLUMN, table_set,
                        TABLE_SET_VISIBILITY_COLUMN, 0,
                        -1);
  }

  {
    guint nb_players   = g_slist_length (_attendees->GetShortList ());
    guint place_offset = 1;

    for (guint i = 0; i < nb_tables-1; i++)
    {
      place_offset = place_offset << 1;

      if ((i == nb_tables-2) && (place_offset << 1) > nb_players)
      {
        break;
      }
      else if ((from_place+place_offset) < nb_players)
      {
        FeedTableSetStore (from_place + place_offset,
                           i+1,
                           &iter);
      }
    }
  }
}

// --------------------------------------------------------------------------------
void TableSupervisor::OnTableSetStatusUpdated (TableSet        *table_set,
                                               TableSupervisor *ts)
{
  GtkTreeIter iter;

  {
    GtkTreePath *path = gtk_tree_row_reference_get_path ((GtkTreeRowReference *) table_set->GetPtrData (ts, "tree_row_ref"));

    gtk_tree_model_get_iter (GTK_TREE_MODEL (ts->_table_set_treestore),
                             &iter,
                             path);
    gtk_tree_path_free (path);
  }

  if (table_set->IsOver ())
  {
    gtk_tree_store_set (ts->_table_set_treestore, &iter,
                        TABLE_SET_STATUS_COLUMN, GTK_STOCK_APPLY,
                        -1);
  }
  else if (table_set->HasError ())
  {
    gtk_tree_store_set (ts->_table_set_treestore, &iter,
                        TABLE_SET_STATUS_COLUMN, GTK_STOCK_DIALOG_WARNING,
                        -1);
  }
  else if (table_set->GetNbTables () > 0)
  {
    gtk_tree_store_set (ts->_table_set_treestore, &iter,
                        TABLE_SET_STATUS_COLUMN, GTK_STOCK_EXECUTE,
                        -1);
  }
  else
  {
    gtk_tree_store_set (ts->_table_set_treestore, &iter,
                        TABLE_SET_STATUS_COLUMN, NULL,
                        -1);
  }

  ts->SignalStatusUpdate ();
}

// --------------------------------------------------------------------------------
void TableSupervisor::OnTableOver (TableSet *table_set,
                                   Table    *table)
{
  GtkTreeIter iter;
  GtkTreeIter defeated_iter;

  {
    GtkTreePath *path = gtk_tree_row_reference_get_path ((GtkTreeRowReference *) table_set->GetPtrData (this, "tree_row_ref"));

    gtk_tree_model_get_iter (GTK_TREE_MODEL (_table_set_treestore),
                             &iter,
                             path);
    gtk_tree_path_free (path);
  }

  if (   (table_set->GetNbTables () >= (table->GetNumber () + 3))
      && gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (_table_set_treestore),
                                        &defeated_iter,
                                        &iter,
                                        table_set->GetNbTables () - table->GetNumber () - 3))
  {
    TableSet *defeated_table_set;

    gtk_tree_model_get (GTK_TREE_MODEL (_table_set_treestore), &defeated_iter,
                        TABLE_SET_TABLE_COLUMN, &defeated_table_set,
                        -1);

    if (defeated_table_set)
    {
      // Store its reference in the table it comes from
      table->_defeated_table_set = defeated_table_set;

      // Populate it
      {
        GSList *loosers;
        GSList *withdrawals;

        table->GetLoosers (&loosers,
                           &withdrawals,
                           NULL);

        defeated_table_set->SetAttendees (loosers,
                                          withdrawals);
      }
    }
  }
}

// --------------------------------------------------------------------------------
void TableSupervisor::Wipe ()
{
  if (_displayed_table_set)
  {
    _displayed_table_set->Wipe ();
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_table_print_toolbutton_clicked (GtkWidget *widget,
                                                                   Object    *owner)
{
  TableSupervisor *t = dynamic_cast <TableSupervisor *> (owner);

  t->OnPrint ();
}

// --------------------------------------------------------------------------------
void TableSupervisor::OnPlugged ()
{
  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("table_classification_toggletoolbutton")),
                                     FALSE);

  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("input_toolbutton")),
                                     FALSE);
}

// --------------------------------------------------------------------------------
void TableSupervisor::OnUnPlugged ()
{
  DeleteTableSets ();
}

// --------------------------------------------------------------------------------
gboolean TableSupervisor::ToggleTableSetLock (GtkTreeModel *model,
                                              GtkTreePath  *path,
                                              GtkTreeIter  *iter,
                                              gboolean      data)
{
  TableSet *table_set;

  gtk_tree_model_get (model, iter,
                      TABLE_SET_TABLE_COLUMN, &table_set,
                      -1);

  if (data)
  {
    table_set->Lock ();
  }
  else
  {
    table_set->UnLock ();
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
void TableSupervisor::OnLocked (Reason reason)
{
  DisableSensitiveWidgets ();

  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("input_toolbutton")),
                                     FALSE);

  gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_treestore),
                          (GtkTreeModelForeachFunc) ToggleTableSetLock,
                          (void *) 1);
}

// --------------------------------------------------------------------------------
void TableSupervisor::OnUnLocked ()
{
  EnableSensitiveWidgets ();

  gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_treestore),
                          (GtkTreeModelForeachFunc) ToggleTableSetLock,
                          0);
}

// --------------------------------------------------------------------------------
void TableSupervisor::OnAttrListUpdated ()
{
  Display ();
}

// --------------------------------------------------------------------------------
void TableSupervisor::FillInConfig ()
{
  Stage::FillInConfig ();

  if (_fenced_places->_value == ALL_PLACES)
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("all_places_radiobutton")),
                                  TRUE);
  }
  else if (_fenced_places->_value == THIRD_PLACES)
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("third_place_radiobutton")),
                                  TRUE);
  }
  else
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("no_radiobutton")),
                                  TRUE);
  }
}

// --------------------------------------------------------------------------------
void TableSupervisor::ApplyConfig ()
{
  Stage::ApplyConfig ();

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("all_places_radiobutton"))))
  {
    _fenced_places->_value = ALL_PLACES;
  }
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("third_place_radiobutton"))))
  {
    _fenced_places->_value = THIRD_PLACES;
  }
  else
  {
    _fenced_places->_value = NONE;
  }

  OnAttrListUpdated ();
  SetTableSetsState ();
}

// --------------------------------------------------------------------------------
gboolean TableSupervisor::StuffTableSet (GtkTreeModel *model,
                                         GtkTreePath  *path,
                                         GtkTreeIter  *iter,
                                         gpointer      data)
{
  TableSet *table_set;

  gtk_tree_model_get (model, iter,
                      TABLE_SET_TABLE_COLUMN, &table_set,
                      -1);

  table_set->OnStuffClicked ();

  return FALSE;
}

// --------------------------------------------------------------------------------
void TableSupervisor::OnStuffClicked ()
{
  gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_treestore),
                          StuffTableSet,
                          NULL);
  OnAttrListUpdated ();
}

// --------------------------------------------------------------------------------
void TableSupervisor::OnInputToggled (GtkWidget *widget)
{
  if (_displayed_table_set)
  {
    _displayed_table_set->OnInputToggled (widget);
  }
}

// --------------------------------------------------------------------------------
void TableSupervisor::OnDisplayToggled (GtkWidget *widget)
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
GSList *TableSupervisor::GetCurrentClassification ()
{
  _result       = NULL;
  _blackcardeds = NULL;

  gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_filter),
                          (GtkTreeModelForeachFunc) GetTableSetClassification,
                          this);

  {
    Player::AttributeId attr_id = Player::AttributeId ("rank", this);

    _result = g_slist_sort_with_data (_result,
                                      (GCompareDataFunc) Player::Compare,
                                      &attr_id);
  }

  _result = g_slist_concat (_result,
                            _blackcardeds);
  _blackcardeds = NULL;

  return _result;
}

// --------------------------------------------------------------------------------
gboolean TableSupervisor::GetTableSetClassification (GtkTreeModel    *model,
                                                     GtkTreePath     *path,
                                                     GtkTreeIter     *iter,
                                                     TableSupervisor *ts)
{
  TableSet *table_set;
  GSList   *current_result;

  gtk_tree_model_get (model, iter,
                      TABLE_SET_TABLE_COLUMN, &table_set,
                      -1);
  current_result = table_set->GetCurrentClassification ();

  for (guint i = 0; i < g_slist_length (current_result); i++)
  {
    Player *player = (Player *) g_slist_nth_data (current_result, i);

    ts->_result = g_slist_remove (ts->_result,
                                  player);
    ts->_result = g_slist_append (ts->_result,
                                  player);
  }

  {
    GSList *blacardeds = table_set->GetBlackcardeds ();

    for (guint i = 0; i < g_slist_length (blacardeds); i++)
    {
      Player *player = (Player *) g_slist_nth_data (blacardeds, i);

      if (g_slist_find (ts->_blackcardeds, player) == NULL)
      {
        ts->_blackcardeds = g_slist_prepend (ts->_blackcardeds,
                                             player);
      }
      ts->_result = g_slist_remove (ts->_result,
                                    player);
    }
    g_slist_free (blacardeds);
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
void TableSupervisor::OnFilterClicked ()
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
void TableSupervisor::OnZoom (gdouble value)
{
  if (_displayed_table_set)
  {
    _displayed_table_set->OnZoom (value);
  }
}

// --------------------------------------------------------------------------------
void TableSupervisor::OnTableSetTreeViewCursorChanged (GtkTreeView *treeview)
{
  GtkTreePath *path;
  GtkTreeIter  iter;
  TableSet    *table_set;

  gtk_tree_view_get_cursor (treeview,
                            &path,
                            NULL);
  gtk_tree_model_get_iter (GTK_TREE_MODEL (_table_set_treestore),
                           &iter,
                           path);
  gtk_tree_path_free (path);

  gtk_tree_model_get (GTK_TREE_MODEL (_table_set_treestore), &iter,
                      TABLE_SET_TABLE_COLUMN, &table_set,
                      -1);

  if (_displayed_table_set != table_set)
  {
    OnTableSetSelected (table_set);
  }
}

// --------------------------------------------------------------------------------
void TableSupervisor::OnPrint ()
{
  if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("table_classification_toggletoolbutton"))))
  {
    Classification *classification = GetClassification ();

    if (classification)
    {
      gchar *title = g_strdup_printf ("%s - %s", gettext ("Table classification"),
                                      GetName ());

      classification->Print (title);
      g_free (title);
    }
  }
  else if (_displayed_table_set)
  {
    _displayed_table_set->OnPrint ();
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_table_filter_toolbutton_clicked (GtkWidget *widget,
                                                                    Object    *owner)
{
  TableSupervisor *t = dynamic_cast <TableSupervisor *> (owner);

  t->OnFilterClicked ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_table_classification_toggletoolbutton_toggled (GtkWidget *widget,
                                                                                  Object    *owner)
{
  TableSupervisor *t = dynamic_cast <TableSupervisor *> (owner);

  t->ToggleClassification (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (widget)));
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_table_stuff_toolbutton_clicked (GtkWidget *widget,
                                                                   Object    *owner)
{
  TableSupervisor *t = dynamic_cast <TableSupervisor *> (owner);

  t->OnStuffClicked ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_input_toolbutton_toggled (GtkWidget *widget,
                                                             Object    *owner)
{
  TableSupervisor *t = dynamic_cast <TableSupervisor *> (owner);

  t->OnInputToggled (widget);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_zoom_scalebutton_value_changed (GtkWidget *widget,
                                                                   gdouble    value,
                                                                   Object    *owner)
{
  TableSupervisor *t = dynamic_cast <TableSupervisor *> (owner);

  t->OnZoom (value);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_display_toolbutton_toggled (GtkWidget *widget,
                                                               Object    *owner)
{
  TableSupervisor *t = dynamic_cast <TableSupervisor *> (owner);

  t->OnDisplayToggled (widget);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_table_set_treeview_cursor_changed (GtkTreeView *treeview,
                                                                      Object      *owner)
{
  TableSupervisor *t = dynamic_cast <TableSupervisor *> (owner);

  t->OnTableSetTreeViewCursorChanged (treeview);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_zoom_hscale_value_changed (GtkRange *range,
                                                              Object   *owner)
{
  TableSupervisor *t = dynamic_cast <TableSupervisor *> (owner);

  t->OnZoom (gtk_range_get_value (range));
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_table_nb_qualified_spinbutton_value_changed (GtkSpinButton *spinbutton,
                                                                                Object        *owner)
{
  Module    *module = dynamic_cast <Module *> (owner);
  Stage     *stage  = dynamic_cast <Stage *> (owner);
  GtkWidget *w      = module->GetWidget ("qualified_ratio_spinbutton");

  g_signal_handlers_disconnect_by_func (G_OBJECT (w),
                                        (void *) on_table_qualified_ratio_spinbutton_value_changed,
                                        owner);
  stage->OnNbQualifiedValueChanged (spinbutton);
  g_signal_connect (G_OBJECT (w), "value-changed",
                    G_CALLBACK (on_table_qualified_ratio_spinbutton_value_changed),
                    owner);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_table_qualified_ratio_spinbutton_value_changed (GtkSpinButton *spinbutton,
                                                                                   Object        *owner)
{
  Module    *module = dynamic_cast <Module *> (owner);
  Stage     *stage  = dynamic_cast <Stage *> (owner);
  GtkWidget *w      = module->GetWidget ("nb_qualified_spinbutton");

  g_signal_handlers_disconnect_by_func (G_OBJECT (w),
                                        (void *) on_table_nb_qualified_spinbutton_value_changed,
                                        owner);
  stage->OnQualifiedRatioValueChanged (spinbutton);
  g_signal_connect (G_OBJECT (w), "value-changed",
                    G_CALLBACK (on_table_nb_qualified_spinbutton_value_changed),
                    owner);
}
