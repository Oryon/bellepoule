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

#include "actors/referees_list.hpp"

#include "enlisted_referee.hpp"
#include "piste.hpp"
#include "job.hpp"
#include "batch.hpp"
#include "referee_pool.hpp"
#include "timeline.hpp"
#include "job_board.hpp"
#include "lasso.hpp"

#include "hall.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  Hall::Hall (RefereePool *referee_pool,
              Listener    *listener)
    : CanvasModule ("hall.glade")
  {
    _listener      = listener;
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

    _lasso     = new Lasso ();
    _clock     = new Clock (this);
    _job_board = new JobBoard ();

    OnTimelineCursorMoved ();
  }

  // --------------------------------------------------------------------------------
  Hall::~Hall ()
  {
    g_list_free_full (_piste_list,
                      (GDestroyNotify) Object::TryToRelease);

    g_list_free_full (_batch_list,
                      (GDestroyNotify) Object::TryToRelease);

    _timeline->Release  ();
    _lasso->Release     ();
    _clock->Release     ();
    _job_board->Release ();
  }

  // --------------------------------------------------------------------------------
  void Hall::OnPlugged ()
  {
    CanvasModule::OnPlugged ();

    g_signal_connect (GetRootItem (),
                      "motion_notify_event",
                      G_CALLBACK (OnMotionNotify),
                      this);
    g_signal_connect (GetRootItem (),
                      "button_press_event",
                      G_CALLBACK (OnButtonPress),
                      this);
    g_signal_connect (GetRootItem (),
                      "button_release_event",
                      G_CALLBACK (OnButtonReleased),
                      this);


    {
      _dnd_config->AddTarget ("bellepoule/referee", GTK_TARGET_SAME_APP|GTK_TARGET_OTHER_WIDGET);
      _dnd_config->AddTarget ("bellepoule/job",     GTK_TARGET_SAME_APP|GTK_TARGET_OTHER_WIDGET);

      _dnd_config->SetOnAWidgetDest (GTK_WIDGET (GetCanvas ()),
                                     GDK_ACTION_COPY);

      ConnectDndDest (GTK_WIDGET (GetCanvas ()));
      EnableDndOnCanvas ();
    }

    {
      AddPiste ();
      AddPiste ();

      AddPiste (Piste::_W + Piste::_H, 0.0);
      AddPiste ();

      AddPiste ((Piste::_W + Piste::_H) / 2, 4 * Piste::_H);

      AddPiste (0.0, 6 * Piste::_H);
      AddPiste ();

      AddPiste (Piste::_W + Piste::_H, 6 * Piste::_H);
      AddPiste ();
    }

    RestoreZoomFactor ();
  }

  // --------------------------------------------------------------------------------
  void Hall::OnNewTime (const gchar *time)
  {
    GtkLabel *clock = GTK_LABEL (_glade->GetWidget ("clock_label"));

    gtk_label_set_text (clock,
                        time);
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
        GList     *selection = batch->GetCurrentSelection ();
        GList     *current   = selection;
        GDateTime *from      = _timeline->RetreiveCursorTime ();

        while (current)
        {
          Job   *job   = (Job *) current->data;
          GList *slots = piste->GetFreeSlots (from, 30 *G_TIME_SPAN_MINUTE);
          Slot  *slot  = (Slot *) slots->data;

          slot->AddJob (job);

          current = g_list_next (current);
        }
        g_date_time_unref (from);

        g_list_free (selection);
        _timeline->Redraw ();
        return;
      }
    }

    {
      EnlistedReferee *referee = dynamic_cast <EnlistedReferee *> (object);

      if (referee)
      {
        Player::AttributeId  weapon_attr_id ("weapon");
        Attribute           *weapon_attr = referee->GetAttribute (&weapon_attr_id);

        piste->AddReferee (referee);
        _referee_pool->RefreshWorkload (weapon_attr->GetStrValue ());
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

        batch->SetProperties (message);
        batch->AttachTo (notebook);
        _timeline->AddBatch (batch);
      }
      else
      {
        batch->SetProperties (message);
      }

      g_free (id);

      _referee_pool->Spread ();
    }
  }

  // --------------------------------------------------------------------------------
  void Hall::DeleteContest (Net::Message *message)
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
  void Hall::ManageFencer (Net::Message *message)
  {
    gchar *id    = message->GetString ("contest");
    Batch *batch = GetBatch (id);

    if (batch)
    {
      batch->ManageFencer (message);
    }

    g_free (id);
  }

  // --------------------------------------------------------------------------------
  void Hall::DeleteJob (Net::Message *message)
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
          _timeline->Redraw ();
          break;
        }

        current = g_list_next (current);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Hall::DeleteFencer (Net::Message *message)
  {
    gchar *id    = message->GetString ("contest");
    Batch *batch = GetBatch (id);

    if (batch)
    {
      batch->DeleteFencer (message);
    }

    g_free (id);
  }

  // --------------------------------------------------------------------------------
  Piste *Hall::AddPiste (gdouble tx, gdouble ty)
  {
    GList *anchor = g_list_last (_piste_list);
    Piste *piste  = new Piste (GetRootItem (), this);

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

    if ((tx != 0.0) || (ty != 0.0))
    {
      piste->Translate (tx,
                        ty);
    }
    else if (anchor)
    {
      piste->AnchorTo ((Piste *) anchor->data);
    }

    {
      GDateTime *cursor = _timeline->RetreiveCursorTime ();

      piste->DisplayAtTime (cursor);
      g_date_time_unref (cursor);
    }

    return piste;
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

    _timeline->Redraw ();
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
  gboolean Hall::OnButtonPress (GooCanvasItem  *item,
                                GooCanvasItem  *target,
                                GdkEventButton *event,
                                Hall           *hall)
  {
    if (event->button == 1)
    {
      hall->CancelSelection ();
      hall->_lasso->Throw (hall->GetRootItem (),
                           event);

      return TRUE;
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  gboolean Hall::OnButtonReleased (GooCanvasItem  *item,
                                   GooCanvasItem  *target,
                                   GdkEventButton *event,
                                   Hall           *hall)
  {
    if (event->button == 1)
    {
      hall->_lasso->Pull ();

      return TRUE;
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void Hall::CancelSelection ()
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
  void Hall::SelectPiste (Piste *piste)
  {
    _selected_list = g_list_prepend (_selected_list,
                                     piste);

    piste->Select ();
  }

  // --------------------------------------------------------------------------------
  void Hall::UnSelectPiste (Piste *piste)
  {
    GList *selected_node = g_list_find (_selected_list, piste);

    _selected_list = g_list_remove_link (_selected_list,
                                         selected_node);
    piste->UnSelect ();
  }

  // --------------------------------------------------------------------------------
  void Hall::OnPisteDoubleClick (Piste          *piste,
                                 GdkEventButton *event)
  {
    GDateTime *cursor = _timeline->RetreiveCursorTime ();
    Slot      *slot   = piste->GetSlotAt (cursor);

    if (slot)
    {
      _job_board->AddJobs (slot->GetJobList ());
      _job_board->Display ();
      _job_board->Clean   ();
    }

    g_date_time_unref (cursor);
  }

  // --------------------------------------------------------------------------------
  void Hall::OnPisteButtonEvent (Piste          *piste,
                                 GdkEventButton *event)
  {
    if (event->type == GDK_BUTTON_PRESS)
    {
      if (g_list_find (_selected_list, piste))
      {
        if (event->state & GDK_CONTROL_MASK)
        {
          UnSelectPiste (piste);
        }
        else
        {
          SetCursor (GDK_FLEUR);
        }
      }
      else
      {
        if ((event->state & GDK_CONTROL_MASK) == 0)
        {
          CancelSelection ();
        }
        SelectPiste (piste);

        SetCursor (GDK_FLEUR);
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
      _lasso->Pull ();

      if (_dragging)
      {
        _dragging = FALSE;

        AlignSelectedOnGrid ();
      }
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
    else
    {
      piste->ConvertFromPisteSpace (&event->x,
                                    &event->y);
      OnCursorMotion (event);
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Hall::OnCursorMotion (GdkEventMotion *event)
  {
    ResetCursor ();

    if (_lasso->OnCursorMotion (event))
    {
      GooCanvasBounds  bounds;
      GList           *current;

      _lasso->GetBounds (&bounds);

      current = _piste_list;
      while (current)
      {
        Piste *piste = (Piste *) current->data;

        UnSelectPiste (piste);
        if (piste->Overlaps (&bounds))
        {
          SelectPiste (piste);
        }

        current = g_list_next (current);
      }
    }

    return TRUE;
  }

  // --------------------------------------------------------------------------------
  gboolean Hall::OnMotionNotify (GooCanvasItem  *item,
                                 GooCanvasItem  *target,
                                 GdkEventMotion *event,
                                 Hall           *hall)
  {
    return hall->OnCursorMotion (event);
  }

  // --------------------------------------------------------------------------------
  gboolean Hall::OnBatchAssignmentRequest (Batch *batch)
  {
    gboolean done = FALSE;

    _listener->OnExposeWeapon (batch->GetWeaponCode ());

    if (_referee_pool->WeaponHasReferees (batch->GetWeaponCode ()) == FALSE)
    {
      GtkWidget *error_dialog = _glade->GetWidget ("referee_error_dialog");

      RunDialog (GTK_DIALOG (error_dialog));
      gtk_widget_hide (error_dialog);
    }
    else
    {
      GtkWidget *dialog = _glade->GetWidget ("job_dialog");

      {
        GtkToggleButton *toggle;

        toggle = GTK_TOGGLE_BUTTON (_glade->GetWidget ("all_jobs"));
        gtk_toggle_button_set_active (toggle, TRUE);
        toggle = GTK_TOGGLE_BUTTON (_glade->GetWidget ("all_referees"));
        gtk_toggle_button_set_active (toggle, TRUE);
        toggle = GTK_TOGGLE_BUTTON (_glade->GetWidget ("all_pistes"));
        gtk_toggle_button_set_active (toggle, TRUE);
        toggle = GTK_TOGGLE_BUTTON (_glade->GetWidget ("now"));
        gtk_toggle_button_set_active (toggle, TRUE);
      }

      done = RunDialog (GTK_DIALOG (dialog)) == 0;
      if (done)
      {
        GList *job_list;
        GList *referee_list;
        GList *current_job;

        // All the jobs?
        {
          GtkToggleButton *all_jobs = GTK_TOGGLE_BUTTON (_glade->GetWidget ("all_jobs"));

          if (gtk_toggle_button_get_active (all_jobs))
          {
            job_list = g_list_copy (batch->GetPendingJobs ());
          }
          else
          {
            job_list = g_list_copy (batch->GetCurrentSelection ());
          }
        }

        // All the referees?
        {
          People::RefereesList *checkin_list = _referee_pool->GetListOf (batch->GetWeaponCode ());
          GtkToggleButton      *all_referees = GTK_TOGGLE_BUTTON (_glade->GetWidget ("all_referees"));

          if (gtk_toggle_button_get_active (all_referees))
          {
            referee_list = g_list_copy (checkin_list->GetList ());
          }
          else
          {
            referee_list = checkin_list->GetSelectedPlayers ();
          }
        }

        current_job = job_list;
        while (current_job)
        {
          Job   *job          = (Job *) current_job->data;
          GList *free_slots   = GetFreePisteSlots (30*G_TIME_SPAN_MINUTE);
          GList *current_slot = free_slots;

          while (current_slot)
          {
            Slot            *slot    = (Slot *) current_slot->data;
            EnlistedReferee *referee;

            referee = GetFreeRefereeFor (referee_list,
                                         slot);
            if (referee)
            {
              slot->Retain ();
              slot->AddJob     (job);
              slot->AddReferee (referee);
              break;
            }

            current_slot = g_list_next (current_slot);
          }
          g_list_free_full (free_slots,
                            (GDestroyNotify) Object::TryToRelease);

          current_job = g_list_next (current_job);
        }

        g_list_free (job_list);
        g_list_free (referee_list);

        _referee_pool->RefreshWorkload (batch->GetWeaponCode ());
        _timeline->Redraw ();
        OnTimelineCursorMoved ();
      }

      gtk_widget_hide (dialog);
    }

    return done;
  }

  // --------------------------------------------------------------------------------
  GList *Hall::GetFreePisteSlots (GTimeSpan duration)
  {
    GDateTime *when;
    GList     *free_slots   = NULL;
    GList     *current      = _piste_list;

    // All the pistes?
    {
      GtkToggleButton *all_pistes = GTK_TOGGLE_BUTTON (_glade->GetWidget ("all_pistes"));

      if (gtk_toggle_button_get_active (all_pistes) == FALSE)
      {
        current = _selected_list;
      }
    }

    // When?
    {
      GtkToggleButton *now = GTK_TOGGLE_BUTTON (_glade->GetWidget ("now"));

      if (gtk_toggle_button_get_active (now))
      {
        when = g_date_time_new_now_local ();
      }
      else
      {
        when = _timeline->RetreiveCursorTime ();
      }
    }


    // Loop over the pistes
    {
      while (current)
      {
        Piste *piste = (Piste *) current->data;

        free_slots = g_list_concat (free_slots,
                                    piste->GetFreeSlots (when,
                                                         30*G_TIME_SPAN_MINUTE));

        current = g_list_next (current);
      }

      g_date_time_unref (when);
    }

    // Sort the result
    free_slots = g_list_sort (free_slots,
                              (GCompareFunc) Slot::CompareAvailbility);

    return free_slots;
  }

  // --------------------------------------------------------------------------------
  EnlistedReferee *Hall::GetFreeRefereeFor (GList *referee_list,
                                            Slot  *slot)
  {
    GList           *current_referee;
    GList           *sorted_list;
    EnlistedReferee *available_referee = NULL;

    sorted_list = g_list_sort (g_list_copy (referee_list),
                               (GCompareFunc) EnlistedReferee::CompareWorkload);

    current_referee = sorted_list;
    while (current_referee)
    {
      EnlistedReferee *referee = (EnlistedReferee *) current_referee->data;

      if (referee->IsAvailableFor (slot))
      {
        available_referee = referee;
        break;
      }

      current_referee = g_list_next (current_referee);
    }

    g_list_free (sorted_list);
    return available_referee;
  }

  // --------------------------------------------------------------------------------
  void Hall::OnTimelineCursorMoved ()
  {
    GDateTime *cursor = _timeline->RetreiveCursorTime ();

    {
      GtkLabel *popup_clock = GTK_LABEL (_glade->GetWidget ("cursor_date"));
      gchar    *text;

      text = g_strdup_printf ("<span weight=\"bold\" background=\"black\" foreground=\"white\"> %02d:%02d </span>",
                              g_date_time_get_hour   (cursor),
                              g_date_time_get_minute (cursor));

      gtk_label_set_markup (popup_clock,
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

    _referee_pool->RefreshWorkload (batch->GetWeaponCode ());
    _timeline->Redraw ();

    _listener->OnExposeWeapon (batch->GetWeaponCode ());
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
}
