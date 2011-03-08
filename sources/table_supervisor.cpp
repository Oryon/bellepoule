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
  TABLE_SET_TABLE_COLUMN
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
  return FALSE;
}

// --------------------------------------------------------------------------------
void TableSupervisor::CreateTableSets ()
{
  guint nb_players = g_slist_length (_attendees->GetShortList ());
  guint nb_levels  = 0;

  for (guint i = 0; i < 32; i++)
  {
    guint bit_cursor;

    bit_cursor = 1;
    bit_cursor = bit_cursor << i;
    if (bit_cursor >= nb_players)
    {
      nb_levels = i++;
      break;
    }
  }

  {
    gtk_tree_store_clear (_table_set_treestore);

    FeedTableSetStore (1,
                       nb_levels,
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
      _displayed_table_set->SetAttendees (_attendees->GetShortList ());
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
      if (strcmp ((char *) xml_node->name, _xml_class_name) == 0)
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
          table_set->Load (xml_node);
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
                                         guint        nb_levels,
                                         GtkTreeIter *parent)
{
  GtkTreeIter  iter;
  gchar       *text = g_strdup_printf ("%d", from_place);

  gtk_tree_store_append (_table_set_treestore,
                         &iter,
                         parent);

  {
    GtkTreePath *path  = gtk_tree_model_get_path (GTK_TREE_MODEL (_table_set_treestore), &iter);
    TableSet    *table_set = new TableSet (this,
                                           gtk_tree_path_to_string (path),
                                           _glade->GetWidget ("from_viewport"));

    gtk_tree_path_free (path);

    table_set->SetDataOwner (this);

    gtk_tree_store_set (_table_set_treestore, &iter,
                        TABLE_SET_NAME_COLUMN, text,
                        TABLE_SET_TABLE_COLUMN, table_set,
                        -1);
  }

  g_free (text);

  {
    guint place_offset = 1;

    for (guint i = 0; i < nb_levels-1; i++)
    {
      place_offset = place_offset << 1;
      FeedTableSetStore (from_place + place_offset,
                         i+1,
                         &iter);
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
void TableSupervisor::OnFromTableComboboxChanged ()
{
  Display ();
}

// --------------------------------------------------------------------------------
void TableSupervisor::OnStuffClicked ()
{
  //toto->OnStuffClicked ();
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
void TableSupervisor::OnSearchMatch ()
{
  if (_displayed_table_set)
  {
    _displayed_table_set->OnSearchMatch ();
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
extern "C" G_MODULE_EXPORT void on_from_table_combobox_changed (GtkWidget *widget,
                                                                Object    *owner)
{
  TableSupervisor *t = dynamic_cast <TableSupervisor *> (owner);

  t->OnFromTableComboboxChanged ();
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
extern "C" G_MODULE_EXPORT void on_quick_search_combobox_changed (GtkWidget *widget,
                                                                  Object    *owner)
{
  TableSupervisor *t = dynamic_cast <TableSupervisor *> (owner);

  t->OnSearchMatch ();
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
