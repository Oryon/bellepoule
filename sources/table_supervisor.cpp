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
  TABLE_SET_STATUS_COLUMN
} DisplayColumnId;

extern "C" G_MODULE_EXPORT void on_from_table_combobox_changed (GtkWidget *widget,
                                                                Object    *owner);

// --------------------------------------------------------------------------------
TableSupervisor::TableSupervisor (StageClass *stage_class)
  : Stage (stage_class),
  Module ("table_supervisor.glade")
{
  _max_score = new Data ("ScoreMax",
                         10);

  _displayed_table_set = NULL;

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

    //filter->ShowAttribute ("previous_stage_rank");
    filter->ShowAttribute ("name");
    //filter->ShowAttribute ("first_name");
    //filter->ShowAttribute ("club");

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
          GetWidget ("main_hook"));
    table_set->Display ();

    _displayed_table_set = table_set;
  }
}

// --------------------------------------------------------------------------------
gboolean TableSupervisor::IsOver ()
{
  GtkTreeIter iter;
  gboolean    iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_table_set_treestore),
                                                             &iter);

  while (iter_is_valid)
  {
    TableSet *table_set;

    gtk_tree_model_get (GTK_TREE_MODEL (_table_set_treestore), &iter,
                        TABLE_SET_TABLE_COLUMN, &table_set,
                        -1);

    if (table_set->IsOver () == FALSE)
    {
      return FALSE;
    }

    iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_table_set_treestore), &iter);
  }

  return TRUE;
}

// --------------------------------------------------------------------------------
void TableSupervisor::CreateTableSets ()
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

  {
    gtk_tree_store_clear (_table_set_treestore);

    FeedTableSetStore (1,
                       nb_tables,
                       NULL);

    gtk_tree_view_expand_all (GTK_TREE_VIEW (_glade->GetWidget ("table_set_treeview")));

  }

  {
    GtkTreeIter iter;

    gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_table_set_treestore),
                                   &iter);
    gtk_tree_model_get (GTK_TREE_MODEL (_table_set_treestore), &iter,
                        TABLE_SET_TABLE_COLUMN, &_displayed_table_set,
                        -1);

    if (_displayed_table_set)
    {
      _displayed_table_set->SetAttendees (g_slist_copy (_attendees->GetShortList ()));
    }
  }
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
  table_set->Release ();

  return FALSE;
}

// --------------------------------------------------------------------------------
void TableSupervisor::DeleteTableSets ()
{
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

  if (_max_score)
  {
    _max_score->Load (xml_node);
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
        CreateTableSets ();
      }
      else if (strcmp ((char *) n->name, "SuiteDeTableaux") == 0)
      {
        TableSet *table_set;
        gchar    *prop = (gchar *) xmlGetProp (n, BAD_CAST "ID");

        table_set = GetTableSet (prop);
        if (table_set)
        {
          table_set->Load (n);
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

  if (_max_score)
  {
    _max_score->Save (xml_writer);
  }
}

// --------------------------------------------------------------------------------
void TableSupervisor::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST _xml_class_name);

  SaveConfiguration (xml_writer);
  SaveAttendees     (xml_writer);

  gtk_tree_model_foreach (GTK_TREE_MODEL (_table_set_treestore),
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
                              _glade->GetWidget ("from_viewport"));

    {
      gchar *text = g_strdup_printf ("Place #%d", from_place);

      table_set->SetName (text);
      g_free (text);
    }

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
                        -1);
  }

  {
    guint place_offset = 1;

    for (guint i = 0; i < nb_tables-1; i++)
    {
      place_offset = place_offset << 1;
      FeedTableSetStore (from_place + place_offset,
                         i+1,
                         &iter);
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
#if 0
  else if (table_set->HasError ())
  {
    gtk_tree_store_set (ts->_table_set_treestore, &iter,
                        TABLE_SET_STATUS_COLUMN, GTK_STOCK_DIALOG_WARNING,
                        -1);
  }
#endif
  else
  {
    gtk_tree_store_set (ts->_table_set_treestore, &iter,
                        TABLE_SET_STATUS_COLUMN, GTK_STOCK_EXECUTE,
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

  {
    guint nb_subtable = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (_table_set_treestore),
                                                        &iter);

    if (   (table->GetColumn () <= nb_subtable-1)
        && gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (_table_set_treestore),
                                          &defeated_iter,
                                          &iter,
                                          nb_subtable - table->GetColumn () - 1))
    {
      TableSet *defeated_table_set;

      gtk_tree_model_get (GTK_TREE_MODEL (_table_set_treestore), &defeated_iter,
                          TABLE_SET_TABLE_COLUMN, &defeated_table_set,
                          -1);

      if (defeated_table_set)
      {
        defeated_table_set->SetAttendees (table->GetLoosers ());
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
void TableSupervisor::OnLocked (Reason reason)
{
  DisableSensitiveWidgets ();

  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("input_toolbutton")),
                                     FALSE);

  //toto->Lock ();
}

// --------------------------------------------------------------------------------
void TableSupervisor::OnUnLocked ()
{
  EnableSensitiveWidgets ();

  //toto->UnLock ();
}

// --------------------------------------------------------------------------------
void TableSupervisor::OnAttrListUpdated ()
{
  Display ();
}

// --------------------------------------------------------------------------------
void TableSupervisor::FillInConfig ()
{
  gchar *text = g_strdup_printf ("%d", _max_score->_value);

  gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("max_score_entry")),
                      text);
  g_free (text);

  gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("name_entry")),
                      GetName ());
}

// --------------------------------------------------------------------------------
void TableSupervisor::ApplyConfig ()
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

        OnAttrListUpdated ();
      }
    }
  }
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
  //return toto->GetCurrentClassification ();
  return NULL;
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
      classification->Print (gettext ("Table round classification"));
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
