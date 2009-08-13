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

#include <gtk/gtk.h>

#include "schedule.hpp"

// --------------------------------------------------------------------------------
Schedule_c::Schedule_c (GtkWidget *stage_name_widget,
                        GtkWidget *previous_widget,
                        GtkWidget *next_widget,
                        GtkWidget *notebook_widget)
{
  _stage_name_widget = stage_name_widget;
  _previous_widget   = previous_widget;
  _next_widget       = next_widget;
  _notebook_widget   = notebook_widget;
  _stage_list        = NULL;
  _current_stage     = NULL;
}

// --------------------------------------------------------------------------------
Schedule_c::~Schedule_c ()
{
  for (guint i = 0; i < g_list_length (_stage_list); i++)
  {
    Object_c::Release ((Stage_c *) g_list_nth_data (_stage_list,
                                                    i));
  }
  g_list_free (_stage_list);
}

// --------------------------------------------------------------------------------
void Schedule_c::Start ()
{
  for (guint i = 0; i < g_list_length (_stage_list); i++)
  {
    Stage_c   *stage;
    GtkWidget *viewport;

    stage = (Stage_c *) g_list_nth_data (_stage_list,
                                         i);
    viewport = gtk_viewport_new (NULL, NULL);
    gtk_notebook_append_page (GTK_NOTEBOOK (_notebook_widget),
                              viewport,
                              gtk_label_new (stage->GetName ()));

    stage->Plug (viewport);
  }

  SetCurrentStage (_current_stage);
}

// --------------------------------------------------------------------------------
void Schedule_c::AddStage (Stage_c *stage)
{
  _stage_list = g_list_append (_stage_list,
                               stage);
}

// --------------------------------------------------------------------------------
void Schedule_c::RemoveStage (Stage_c *stage)
{
  _stage_list = g_list_remove (_stage_list,
                               stage);
}

// --------------------------------------------------------------------------------
void Schedule_c::NextStage ()
{
}

// --------------------------------------------------------------------------------
void Schedule_c::PreviousStage ()
{
}

// --------------------------------------------------------------------------------
void Schedule_c::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "stage",
                                     "%d", _current_stage);
}

// --------------------------------------------------------------------------------
void Schedule_c::Load (xmlNode *xml_node)
{
  gchar *attr;

  attr = (gchar *) xmlGetProp (xml_node, BAD_CAST "stage");
  if (attr)
  {
    guint  stage = atoi (attr);
    GList *current_stage;

    current_stage = g_list_nth (_stage_list,
                                stage);
    SetCurrentStage (current_stage);
  }
}

// --------------------------------------------------------------------------------
void Schedule_c::SetCurrentStage (GList *stage)
{
  _current_stage = stage;

  if (g_list_previous (_current_stage) == NULL)
  {
    gtk_widget_set_sensitive (_previous_widget,
                              false);
  }
  else
  {
    gtk_widget_set_sensitive (_previous_widget,
                              true);
  }

  if (g_list_next (_current_stage) == NULL)
  {
    gtk_widget_set_sensitive (_next_widget,
                              false);
  }
  else
  {
    gtk_widget_set_sensitive (_next_widget,
                              true);
  }

  {
    Stage_c *stage = (Stage_c *) _current_stage->data;

    gtk_entry_set_text (GTK_ENTRY (_stage_name_widget),
                        stage->GetName ());
  }
}

// --------------------------------------------------------------------------------
void Schedule_c::CancelCurrentStage ()
{
}
