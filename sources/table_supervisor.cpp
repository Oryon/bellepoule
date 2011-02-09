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

Table *toto = NULL;

const gchar *TableSupervisor::_class_name     = N_("Table");
const gchar *TableSupervisor::_xml_class_name = "PhaseDeTableaux";

typedef enum
{
  FROM_NAME_COLUMN,
  FROM_STATUS_COLUMN
} FromColumnId;

typedef enum
{
  DISPLAY_NAME_COLUMN
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

  _display_treestore = GTK_TREE_STORE (_glade->GetObject ("display_treestore"));
}

// --------------------------------------------------------------------------------
TableSupervisor::~TableSupervisor ()
{
  _max_score->Release ();

  DeleteSubTables ();
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

  Plug (toto,
        GetWidget ("main_hook"));
  toto->Display ();
}

// --------------------------------------------------------------------------------
gboolean TableSupervisor::IsOver ()
{
  return FALSE;
}

// --------------------------------------------------------------------------------
void TableSupervisor::CreateSubTables ()
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
    gtk_tree_store_clear (_display_treestore);

    FeedDisplayStore (1,
                      nb_levels-1,
                      NULL);

    gtk_tree_view_expand_all (GTK_TREE_VIEW (_glade->GetWidget ("display_treeview")));
  }
}

// --------------------------------------------------------------------------------
void TableSupervisor::DeleteSubTables ()
{
  gtk_tree_store_clear (_display_treestore);
}

// --------------------------------------------------------------------------------
void TableSupervisor::Garnish ()
{
  DeleteSubTables ();
  CreateSubTables ();

  {
    toto = new Table (this);

    toto->SetAttendees (_attendees->GetShortList ());
    toto->SetDataOwner (this);
  }
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
      else if (strcmp ((char *) n->name, "SuiteDeTableaux") == 0)
      {
        if (_attendees)
        {
          CreateSubTables ();
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
  SaveAttendees (xml_writer);

  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void TableSupervisor::FeedDisplayStore (guint        from_place,
                                        guint        nb_levels,
                                        GtkTreeIter *parent)
{
  GtkTreeIter  iter;
  gchar       *text = g_strdup_printf ("%d", from_place);

  gtk_tree_store_append (_display_treestore,
                         &iter,
                         parent);

  gtk_tree_store_set (_display_treestore, &iter,
                      DISPLAY_NAME_COLUMN, text,
                      -1);
  g_free (text);

  {
    guint place_offset = 1;

    for (guint i = 0; i < nb_levels-1; i++)
    {
      place_offset = place_offset << 1;
      FeedDisplayStore (from_place + place_offset,
                        i+1,
                        &iter);
    }
  }
}

// --------------------------------------------------------------------------------
void TableSupervisor::Wipe ()
{
  toto->Wipe ();
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
  DeleteSubTables ();
}

// --------------------------------------------------------------------------------
void TableSupervisor::OnLocked (Reason reason)
{
  DisableSensitiveWidgets ();

  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("input_toolbutton")),
                                     FALSE);

  toto->Lock ();
}

// --------------------------------------------------------------------------------
void TableSupervisor::OnUnLocked ()
{
  EnableSensitiveWidgets ();

  toto->UnLock ();
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
  toto->OnStuffClicked ();
  OnAttrListUpdated ();
}

// --------------------------------------------------------------------------------
void TableSupervisor::OnInputToggled (GtkWidget *widget)
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
  toto->OnZoom (value);
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
  else
  {
    toto->OnPrint ();
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
