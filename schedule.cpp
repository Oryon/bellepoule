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
                        GtkWidget *next_widget)
{
  _stage_name_widget = stage_name_widget;
  _previous_widget   = previous_widget;
  _next_widget       = next_widget;

  for (gint i = 0; i < NB_STAGE; i++)
  {
    _subscriber[i] = NULL;
  }

  _current_stage = CHECKIN;
}

// --------------------------------------------------------------------------------
Schedule_c::~Schedule_c ()
{
  for (gint i = 0; i < NB_STAGE; i++)
  {
    g_slist_free (_subscriber[i]);
  }
}

// --------------------------------------------------------------------------------
void Schedule_c::Start ()
{
  SetCurrentStage (_current_stage);
}

// --------------------------------------------------------------------------------
void Schedule_c::NextStage ()
{
  if (_current_stage < OVER)
  {
    SetCurrentStage ((stage_t) ((gint) (_current_stage) + 1));
  }
}

// --------------------------------------------------------------------------------
void Schedule_c::PreviousStage ()
{
  if (_current_stage > 0)
  {
    CancelCurrentStage ();
    SetCurrentStage ((stage_t) ((gint) (_current_stage) - 1));
  }
}

// --------------------------------------------------------------------------------
Schedule_c::stage_t Schedule_c::GetCurrentStage ()
{
  return _current_stage;
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
    stage_t stage = (stage_t) atoi (attr);

    for (gint i = 0; i <= stage; i++)
    {
      SetCurrentStage ((stage_t) i);
    }
  }
}

// --------------------------------------------------------------------------------
gchar *Schedule_c::GetStageName (stage_t stage)
{
  switch (stage)
  {
    case CHECKIN:
      {
        return "CHECKIN";
      }
      break;

    case POOL:
      {
        return "POOL";
      }
      break;

    case POOL_ALLOCATION:
      {
        return "POOL_ALLOCATION";
      }
      break;

    case TABLE:
      {
        return "TABLE";
      }
      break;

    case OVER:
      {
        return "OVER";
      }
      break;

    default: break;
  }
  return "????";
}

// --------------------------------------------------------------------------------
void Schedule_c::SetCurrentStage (stage_t stage)
{
  {
    Object_c     *client;
    Subscriber_t *subscriber;
    StageEvent_t  cbk;

    for (guint i = 0; i < g_slist_length (_subscriber[_current_stage]); i++)
    {
      subscriber = (Subscriber_t *) g_slist_nth_data (_subscriber[_current_stage], i);
      client     = subscriber->client;

      cbk = subscriber->OnStageLeaved;
      (client->*cbk) ();
    }

    for (guint i = 0; i < g_slist_length (_subscriber[stage]); i++)
    {
      subscriber = (Subscriber_t *) g_slist_nth_data (_subscriber[stage], i);
      client     = subscriber->client;

      cbk = subscriber->OnStageEntered;
      (client->*cbk) ();
    }
  }

  _current_stage = stage;

  if (_current_stage == 0)
  {
    gtk_widget_set_sensitive (_previous_widget,
                              false);
  }
  else
  {
    gtk_widget_set_sensitive (_previous_widget,
                              true);
  }

  if (_current_stage == NB_STAGE - 1)
  {
    gtk_widget_set_sensitive (_next_widget,
                              false);
  }
  else
  {
    gtk_widget_set_sensitive (_next_widget,
                              true);
  }

  gtk_entry_set_text (GTK_ENTRY (_stage_name_widget),
                      GetStageName (_current_stage));
}

// --------------------------------------------------------------------------------
void Schedule_c::CancelCurrentStage ()
{
}

// --------------------------------------------------------------------------------
void Schedule_c::Subscribe (Object_c     *client,
                            stage_t       stage,
                            StageEvent_t  stage_entered,
                            StageEvent_t  stage_leaved)
{
  Subscriber_t *subscriber_cbk = new Subscriber_t;

  subscriber_cbk->client         = client;
  subscriber_cbk->OnStageEntered = stage_entered;
  subscriber_cbk->OnStageLeaved  = stage_leaved;

  _subscriber[stage] = g_slist_append (_subscriber[stage],
                                       subscriber_cbk);
}

// --------------------------------------------------------------------------------
void Schedule_c::UnSubscribe (Object_c *client)
{
  for (gint i = 0; i < NB_STAGE; i++)
  {
    GSList *node = _subscriber[i];

    while (node)
    {
      GSList       *next_node;
      Subscriber_t *subscriber;

      next_node = g_slist_next (node);

      subscriber = (Subscriber_t *) node->data;
      if (subscriber->client == client)
      {
        _subscriber[i] = g_slist_delete_link (_subscriber[i],
                                              node);
        delete subscriber;
      }

      node = next_node;
    }
  }
}
