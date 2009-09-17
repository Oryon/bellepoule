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

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <gtk/gtk.h>

#include "schedule.hpp"

// --------------------------------------------------------------------------------
Schedule_c::Schedule_c ()
: Module_c ("schedule.glade",
            "schedule_notebook")
{
  _stage_list    = NULL;
  _current_stage = NULL;

  _glade->Bind ("previous_stage_toolbutton", this);
  _glade->Bind ("next_stage_toolbutton",     this);
}

// --------------------------------------------------------------------------------
Schedule_c::~Schedule_c ()
{
  for (guint i = 0; i < g_list_length (_stage_list); i++)
  {
    Stage_c *stage;

    stage = ((Stage_c *) g_list_nth_data (_stage_list,
                                          i));
    Object_c::Release (stage);
  }
  g_list_free (_stage_list);
}

// --------------------------------------------------------------------------------
void Schedule_c::AddStage (Stage_c *stage)
{
  if (stage)
  {
    {
      Stage_c *previous = NULL;

      if (_stage_list)
      {
        previous = (Stage_c *) (g_list_last (_stage_list)->data);
      }

      stage->SetPrevious (previous);

      _stage_list = g_list_append (_stage_list,
                                   stage);
      if (previous == NULL)
      {
        _current_stage = _stage_list;
      }
    }

    {
      Module_c  *module   = (Module_c *) dynamic_cast <Module_c *> (stage);
      GtkWidget *viewport = gtk_viewport_new (NULL, NULL);
      gchar     *name     = stage->GetFullName ();

      gtk_notebook_append_page (GTK_NOTEBOOK (GetRootWidget ()),
                                viewport,
                                gtk_label_new (name));
      g_free (name);

      Plug (module,
            viewport);
    }
  }
}

// --------------------------------------------------------------------------------
void Schedule_c::RemoveStage (Stage_c *stage)
{
  _stage_list = g_list_remove (_stage_list,
                               stage);
}

// --------------------------------------------------------------------------------
void Schedule_c::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST "schedule");
  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "current_stage",
                                     "%d", g_list_position (_stage_list,
                                                            _current_stage));

  for (guint i = 0; i < g_list_length (_stage_list); i++)
  {
    Stage_c *stage;

    stage = ((Stage_c *) g_list_nth_data (_stage_list,
                                          i));
    stage->Save (xml_writer);
  }
  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void Schedule_c::Load (xmlDoc *doc)
{
  xmlXPathContext *xml_context = xmlXPathNewContext (doc);
  guint            current_stage_index = 0;
  Stage_c         *stage = (Stage_c *) g_list_nth_data (_stage_list,
                                                        0);

  {
    xmlXPathObject *xml_object  = xmlXPathEval (BAD_CAST "/contest/schedule", xml_context);
    xmlNodeSet     *xml_nodeset = xml_object->nodesetval;
    char           *attr        = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0],
                                                        BAD_CAST "current_stage");

    if (attr)
    {
      current_stage_index = atoi (attr);
    }

    xmlXPathFreeObject  (xml_object);
  }

  {
    xmlXPathObject *xml_object  = xmlXPathEval (BAD_CAST "/contest/schedule/*", xml_context);
    xmlNodeSet     *xml_nodeset = xml_object->nodesetval;

    for (guint i = 0; i < (guint) xml_nodeset->nodeNr; i++)
    {
      if (i < current_stage_index)
      {
        stage->Lock ();
      }

      stage = Stage_c::CreateInstance (xml_nodeset->nodeTab[i]);
      if (stage)
      {
        gchar *attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[i],
                                            BAD_CAST "name");
        stage->SetName (attr);
        AddStage (stage);
        stage->Load (xml_nodeset->nodeTab[i]);
      }
    }

    xmlXPathFreeObject  (xml_object);
  }

  xmlXPathFreeContext (xml_context);

  gtk_widget_show_all (GetRootWidget ());
  SetCurrentStage (g_list_nth (_stage_list,
                               current_stage_index));
}

// --------------------------------------------------------------------------------
void Schedule_c::SetCurrentStage (GList *stage)
{
  _current_stage = stage;

  if (_current_stage)
  {
    if (g_list_previous (_current_stage) == NULL)
    {
      gtk_widget_set_sensitive (_glade->GetWidget ("previous_stage_toolbutton"),
                                false);
    }
    else
    {
      gtk_widget_set_sensitive (_glade->GetWidget ("previous_stage_toolbutton"),
                                true);
    }

    if (g_list_next (_current_stage) == NULL)
    {
      gtk_widget_set_sensitive (_glade->GetWidget ("next_stage_toolbutton"),
                                false);
    }
    else
    {
      gtk_widget_set_sensitive (_glade->GetWidget ("next_stage_toolbutton"),
                                true);
    }

    {
      Stage_c *stage = (Stage_c *) _current_stage->data;

      gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("stage_entry")),
                          stage->GetName ());
    }
  }

  {
    gint current_position = g_list_position (_stage_list,
                                             _current_stage);

    gtk_notebook_set_current_page  (GTK_NOTEBOOK (GetRootWidget ()),
                                    current_position);
  }
}

// --------------------------------------------------------------------------------
void Schedule_c::CancelCurrentStage ()
{
}

// --------------------------------------------------------------------------------
void Schedule_c::OnPlugged ()
{
  GtkToolbar *toolbar = GetToolbar ();
  GtkWidget  *w;

  w = _glade->GetWidget ("previous_stage_toolbutton");
  _glade->DetachFromParent (w);
  gtk_toolbar_insert (toolbar,
                      GTK_TOOL_ITEM (w),
                      -1);

  w = _glade->GetWidget ("stage_name_toolbutton");
  _glade->DetachFromParent (w);
  gtk_toolbar_insert (toolbar,
                      GTK_TOOL_ITEM (w),
                      -1);

  w = _glade->GetWidget ("next_stage_toolbutton");
  _glade->DetachFromParent (w);
  gtk_toolbar_insert (toolbar,
                      GTK_TOOL_ITEM (w),
                      -1);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_previous_stage_toolbutton_clicked (GtkWidget *widget,
                                                                      GdkEvent  *event,
                                                                      gpointer  *data)
{
  Schedule_c *s = (Schedule_c *) g_object_get_data (G_OBJECT (widget),
                                                    "instance");
  s->on_previous_stage_toolbutton_clicked ();
}

// --------------------------------------------------------------------------------
void Schedule_c::on_previous_stage_toolbutton_clicked ()
{
  Stage_c *stage;

  stage = (Stage_c *) _current_stage->data;
  stage->Wipe ();

  SetCurrentStage (g_list_previous (_current_stage));

  stage = (Stage_c *) _current_stage->data;
  stage->UnLock ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_next_stage_toolbutton_clicked (GtkWidget *widget,
                                                                  GdkEvent  *event,
                                                                  gpointer  *data)
{
  Schedule_c *s = (Schedule_c *) g_object_get_data (G_OBJECT (widget),
                                                    "instance");
  s->on_next_stage_toolbutton_clicked ();
}

// --------------------------------------------------------------------------------
void Schedule_c::on_next_stage_toolbutton_clicked ()
{
  Stage_c *stage;

  stage = (Stage_c *) _current_stage->data;
  stage->Lock ();

  SetCurrentStage (g_list_next (_current_stage));

  stage = (Stage_c *) _current_stage->data;
  stage->Enter ();
}
