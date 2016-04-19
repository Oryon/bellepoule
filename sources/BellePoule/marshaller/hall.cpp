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

#include "actors/referee.hpp"
#include "job.hpp"
#include "batch.hpp"
#include "referee_pool.hpp"
#include "timeline.hpp"

#include "hall.hpp"

// --------------------------------------------------------------------------------
Hall::Hall (RefereePool *referee_pool)
  : CanvasModule ("hall.glade")
{
  _piste_list    = NULL;
  _selected_list = NULL;

  _dragging = FALSE;

  SetZoomer (GTK_RANGE (_glade->GetWidget ("zoom_scale")),
             1.5);

  _batch_list = NULL;

  _referee_pool = referee_pool;

  {
    _timeline = new Timeline (this);

    Plug (_timeline,
          _glade->GetWidget ("timeline_viewport"));
  }

  OnTimelineCursorMoved ();
}

// --------------------------------------------------------------------------------
Hall::~Hall ()
{
  g_list_free_full (_piste_list,
                    (GDestroyNotify) Object::TryToRelease);

  g_list_free_full (_batch_list,
                    (GDestroyNotify) Object::TryToRelease);

  _timeline->Release ();
}

// --------------------------------------------------------------------------------
void Hall::OnPlugged ()
{
  CanvasModule::OnPlugged ();

  _root = goo_canvas_group_new (GetRootItem (),
                                NULL);

  g_signal_connect (_root,
                    "motion_notify_event",
                    G_CALLBACK (OnMotionNotify),
                    this);

#if 1
  {
    GooCanvasItem *rect = goo_canvas_rect_new (_root,
                                               0.0, 0.0, 300, 150,
                                               "line-width", 0.0,
                                               "fill-color", "White",
                                               NULL);
    g_signal_connect (rect,
                      "motion_notify_event",
                      G_CALLBACK (OnMotionNotify),
                      this);
  }
#endif

  g_signal_connect (GetRootItem (),
                    "button_press_event",
                    G_CALLBACK (OnSelected),
                    this);


  {
    _dnd_config->AddTarget ("bellepoule/referee", GTK_TARGET_SAME_APP|GTK_TARGET_OTHER_WIDGET);
    _dnd_config->AddTarget ("bellepoule/job",     GTK_TARGET_SAME_APP|GTK_TARGET_OTHER_WIDGET);

    _dnd_config->SetOnAWidgetDest (GTK_WIDGET (GetCanvas ()),
                                   GDK_ACTION_COPY);

    ConnectDndDest (GTK_WIDGET (GetCanvas ()));
    EnableDndOnCanvas ();
  }

  AddPiste ();
  RestoreZoomFactor ();
}

// --------------------------------------------------------------------------------
Object *Hall::GetDropObjectFromRef (guint32 ref,
                                    guint   key)
{
  if (g_strcmp0 (g_quark_to_string (key), "bellepoule/referee") == 0)
  {
    return _referee_pool->GetReferee (ref);
  }
  else if (g_strcmp0 (g_quark_to_string (key), "bellepoule/job") == 0)
  {
    return GetBatch (ref);
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void Hall::DropObject (Object   *object,
                       DropZone *source_zone,
                       DropZone *target_zone)
{
  Piste *piste = dynamic_cast <Piste *> (target_zone);

  {
    Batch *batch = dynamic_cast <Batch *> (object);

    if (batch)
    {
      GSList *selection = batch->GetCurrentSelection ();
      GSList *current   = selection;

      while (current)
      {
        Job      *job      = (Job *) current->data;
        TimeSlot *timeslot = piste->GetFreeTimeslot (30*G_TIME_SPAN_MINUTE);

        timeslot->AddJob (job);

        current = g_slist_next (current);
      }

      g_slist_free (selection);
      Timeline::Redraw (_timeline);
      return;
    }
  }

  {
    Referee *refere = dynamic_cast <Referee *> (object);

    if (refere)
    {
      piste->AddReferee (refere);
    }
  }
}

// --------------------------------------------------------------------------------
void Hall::ManageContest (Net::Message *message,
                          GtkNotebook  *notebook)
{
  gchar *id = message->GetString ("uuid");

  if (id)
  {
    Batch *batch = GetBatch (id);

    if (batch == NULL)
    {
      batch = new Batch (id,
                         this);

      _batch_list = g_list_prepend (_batch_list,
                                    batch);

      batch->AttachTo (notebook);
      _timeline->AddBatch (batch);
    }

    batch->SetProperties (message);

    g_free (id);

    _referee_pool->Spread ();
  }
}

// --------------------------------------------------------------------------------
void Hall::DropContest (Net::Message *message)
{
  gchar *id = message->GetString ("uuid");

  if (id)
  {
    Batch *batch = GetBatch (id);

    if (batch)
    {
      GList *node = g_list_find (_batch_list,
                                 batch);

      _batch_list = g_list_delete_link (_batch_list,
                                        node);

      _timeline->RemoveBatch (batch);
      batch->Release ();
    }

    g_free (id);
  }
}

// --------------------------------------------------------------------------------
void Hall::ManageJob (Net::Message *message)
{
  gchar *id    = message->GetString ("contest");
  Batch *batch = GetBatch (id);

  if (batch)
  {
    batch->Load (message);
  }

  g_free (id);
}

// --------------------------------------------------------------------------------
void Hall::DropJob (Net::Message *message)
{
  guint32 contest_id;

  {
    gchar *id = message->GetString ("contest");

    contest_id = (guint32) g_ascii_strtoull (id,
                                             NULL,
                                             16);
    g_free (id);
  }

  {
    GList *current = _batch_list;

    while (current)
    {
      Batch *batch = (Batch *) current->data;

      if (batch->GetId () == contest_id)
      {
        batch->RemoveJob (message);
        break;
      }

      current = g_list_next (current);
    }
  }
}

// --------------------------------------------------------------------------------
void Hall::AddPiste ()
{
  GList *anchor = g_list_last (_piste_list);
  Piste *piste  = new Piste (_root, this);

  piste->SetListener (this);

  _drop_zones = g_slist_append (_drop_zones,
                                piste);

  {
    GList *before = _piste_list;

    for (guint i = 1; before != NULL; i++)
    {
      Piste *current_piste = (Piste *) before->data;

      if (current_piste->GetId () != i)
      {
        break;
      }
      piste->SetId (i+1);

      before = g_list_next (before);
    }

    if (before)
    {
      _piste_list = g_list_insert_before (_piste_list,
                                          before,
                                          piste);
    }
    else
    {
      _piste_list = g_list_append (_piste_list,
                                   piste);
    }
  }

  if (anchor)
  {
    piste->AnchorTo ((Piste *) anchor->data);
  }
  else
  {
    piste->Translate (0.5,
                      0.5);
  }

  {
    GDateTime *cursor = _timeline->RetreiveCursorTime ();

    piste->DisplayAtTime (cursor);
    g_date_time_unref (cursor);
  }
}

// --------------------------------------------------------------------------------
Batch *Hall::GetBatch (const gchar *id)
{
  guint32 dnd_id = (guint32) g_ascii_strtoull (id,
                                               NULL,
                                               16);
  return GetBatch (dnd_id);
}

// --------------------------------------------------------------------------------
Batch *Hall::GetBatch (guint32 id)
{
  GList *current = _batch_list;

  while (current)
  {
    Batch *batch = (Batch *) current->data;

    if (batch->GetId () == id)
    {
      return batch;
    }

    current = g_list_next (current);
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void Hall::RemoveSelected ()
{
  GList *current = _selected_list;

  while (current)
  {
    Piste *piste = (Piste *) current->data;

    RemovePiste (piste);

    current = g_list_next (current);
  }

  g_list_free (_selected_list);
  _selected_list = NULL;

  Timeline::Redraw (_timeline);
}

// --------------------------------------------------------------------------------
void Hall::AlignSelectedOnGrid ()
{
  GList *current = _selected_list;

  while (current)
  {
    Piste *piste = (Piste *) current->data;

    piste->AlignOnGrid ();

    current = g_list_next (current);
  }
}

// --------------------------------------------------------------------------------
void Hall::TranslateSelected (gdouble tx,
                              gdouble ty)
{
  GList *current = _selected_list;

  while (current)
  {
    Piste *piste = (Piste *) current->data;

    piste->Translate (tx,
                      ty);

    current = g_list_next (current);
  }
}

// --------------------------------------------------------------------------------
void Hall::RotateSelected ()
{
  GList *current = _selected_list;

  while (current)
  {
    Piste *piste = (Piste *) current->data;

    piste->Rotate ();

    current = g_list_next (current);
  }
}

// --------------------------------------------------------------------------------
void Hall::RemovePiste (Piste *piste)
{
  if (piste)
  {
    GList *node = g_list_find (_piste_list, piste);

    if (node)
    {
      GSList *zone = g_slist_find (_drop_zones, piste);

      if (zone)
      {
        _drop_zones = g_slist_remove_link (_drop_zones,
                                           zone);
      }

      _piste_list = g_list_remove_link (_piste_list,
                                        node);

      piste->Disable ();
      piste->Release ();
    }
  }
}

// --------------------------------------------------------------------------------
gboolean Hall::OnSelected (GooCanvasItem  *item,
                           GooCanvasItem  *target,
                           GdkEventButton *event,
                           Hall           *hall)
{
  hall->CancelSeletion ();

  return TRUE;
}

// --------------------------------------------------------------------------------
void Hall::CancelSeletion ()
{
  GList *current = _selected_list;

  while (current)
  {
    Piste *current_piste = (Piste *) current->data;

    current_piste->UnSelect ();

    current = g_list_next (current);
  }

  g_list_free (_selected_list);
  _selected_list = NULL;
}

// --------------------------------------------------------------------------------
void Hall::OnPisteButtonEvent (Piste          *piste,
                               GdkEventButton *event)
{
  if (event->type == GDK_BUTTON_PRESS)
  {
    GList *selected_node = g_list_find (_selected_list, piste);

    if (selected_node)
    {
      if (event->state & GDK_CONTROL_MASK)
      {
        _selected_list = g_list_remove_link (_selected_list,
                                             selected_node);
        piste->UnSelect ();
      }
    }
    else
    {
      if ((event->state & GDK_CONTROL_MASK) == 0)
      {
        CancelSeletion ();
      }

      {
        _selected_list = g_list_prepend (_selected_list,
                                         piste);

        piste->Select ();
        SetCursor (GDK_FLEUR);
      }
    }

    {
      _dragging = TRUE;
      _drag_x = event->x;
      _drag_y = event->y;

      piste->GetHorizontalCoord (&_drag_x,
                                 &_drag_y);
    }
  }
  else
  {
    _dragging = FALSE;

    AlignSelectedOnGrid ();
  }
}

// --------------------------------------------------------------------------------
void Hall::OnPisteMotionEvent (Piste          *piste,
                               GdkEventMotion *event)
{
  SetCursor (GDK_FLEUR);

  if (_dragging)
  {
    if (   (event->state & GDK_BUTTON1_MASK)
        && ((event->state & GDK_CONTROL_MASK) == 0))
    {
      double new_x = event->x;
      double new_y = event->y;

      piste->GetHorizontalCoord (&new_x,
                                 &new_y);

      TranslateSelected (new_x - _drag_x,
                         new_y - _drag_y);
    }
  }
  else if (g_list_find (_selected_list, piste) == NULL)
  {
    ResetCursor ();
  }
}

// --------------------------------------------------------------------------------
gboolean Hall::OnMotionNotify (GooCanvasItem  *item,
                               GooCanvasItem  *target,
                               GdkEventMotion *event,
                               Hall           *hall)
{
  hall->ResetCursor ();

  return TRUE;
}

// --------------------------------------------------------------------------------
void Hall::OnBatchAssignmentRequest (Batch *batch)
{
  GtkWidget *dialog = _glade->GetWidget ("job_dialog");

  {
    GtkSpinButton *hour   = GTK_SPIN_BUTTON (_glade->GetWidget ("hour_spinbutton"));
    GtkSpinButton *minute = GTK_SPIN_BUTTON (_glade->GetWidget ("minute_spinbutton"));

    GDateTime *time = g_date_time_new_now_local ();

    gtk_spin_button_set_value  (hour,   g_date_time_get_hour   (time));
    gtk_spin_button_set_value  (minute, g_date_time_get_minute (time));
  }

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == 0)
  {
    if (_piste_list)
    {
      GList *pending_jobs    = g_list_copy (batch->GetPendingJobs ());
      GList *current_job     = pending_jobs;
      GList *current_piste   = NULL;

      while (current_job)
      {
        Job     *job     = (Job *) current_job->data;
        Referee *referee = _referee_pool->GetRefereeFor (job);

        if (referee)
        {
          if (current_piste == NULL)
          {
            _piste_list = g_list_sort (_piste_list,
                                       (GCompareFunc) Piste::CompareAvailbility);
            current_piste = _piste_list;
          }

          {
            Piste    *piste    = (Piste *) current_piste->data;
            TimeSlot *timeslot = piste->GetFreeTimeslot (30*G_TIME_SPAN_MINUTE);

            timeslot->AddJob     (job);
            timeslot->AddReferee (referee);
          }

          current_piste = g_list_next (current_piste);
        }
        current_job   = g_list_next (current_job);
      }

      g_list_free (pending_jobs);

      Timeline::Redraw (_timeline);
      OnTimelineCursorMoved ();
    }
  }

  gtk_widget_hide (dialog);
}

// --------------------------------------------------------------------------------
void Hall::OnTimelineCursorMoved ()
{
  GDateTime *cursor = _timeline->RetreiveCursorTime ();

  {
    GtkLabel *clock = GTK_LABEL (_glade->GetWidget ("clock_label"));
    gchar    *text;

    text = g_strdup_printf ("%02d:%02d", g_date_time_get_hour (cursor),
                                         g_date_time_get_minute (cursor));

    gtk_label_set_text (clock,
                        text);

    g_free (text);
  }

  {
    GList *current_piste = _piste_list;

    while (current_piste)
    {
      Piste *piste = (Piste *) current_piste->data;

      piste->DisplayAtTime (cursor);

      current_piste = g_list_next (current_piste);
    }
  }

  g_date_time_unref (cursor);
}

// --------------------------------------------------------------------------------
void Hall::OnBatchAssignmentCancel (Batch *batch)
{
  GList *current_piste = _piste_list;

  while (current_piste)
  {
    Piste *piste = (Piste *) current_piste->data;

    piste->RemoveBatch (batch);

    current_piste = g_list_next (current_piste);
  }

  Timeline::Redraw (_timeline);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_add_piste_button_clicked (GtkWidget *widget,
                                                             Object    *owner)
{
  Hall *h = dynamic_cast <Hall *> (owner);

  h->AddPiste ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_rotate_piste_button_clicked (GtkWidget *widget,
                                                                Object    *owner)
{
  Hall *h = dynamic_cast <Hall *> (owner);

  h->RotateSelected ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_remove_piste_button_clicked (GtkWidget *widget,
                                                                Object    *owner)
{
  Hall *h = dynamic_cast <Hall *> (owner);

  h->RemoveSelected ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT gchar *on_timeline_format_value (GtkScale *scale,
                                                            gdouble   value)
{
  return g_strdup_printf ("T0+%02d\'", (guint) value);
}
