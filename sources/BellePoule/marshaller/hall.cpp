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

#include "util/fie_time.hpp"
#include "util/glade.hpp"
#include "util/dnd_config.hpp"
#include "util/attribute.hpp"
#include "util/attribute_desc.hpp"
#include "util/filter.hpp"
#include "util/player.hpp"
#include "actors/referees_list.hpp"
#include "application/weapon.hpp"

#include "enlisted_referee.hpp"
#include "piste.hpp"
#include "job.hpp"
#include "batch.hpp"
#include "competition.hpp"
#include "referee_pool.hpp"
#include "timeline.hpp"
#include "job_board.hpp"
#include "lasso.hpp"
#include "affinities.hpp"

#include "hall.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  Hall::Hall (RefereePool *referee_pool,
              Listener    *listener)
    : Object ("Hall"),
    CanvasModule ("hall.glade")
  {
    _listener            = listener;
    _piste_list          = NULL;
    _selected_piste_list = NULL;
    _redraw_timeout      = 0;
    _floating_job        = NULL;

    _dragging = FALSE;

    SetZoomer (GTK_RANGE (_glade->GetWidget ("zoom_scale")));

    _competition_list = NULL;

    _referee_pool = referee_pool;
    _referee_pool->SetDndPeerListener (this);

    _lasso = new Lasso ();
    _clock = new Clock (this);

    {
      _timeline = new Timeline (_clock,
                                this);

      Plug (_timeline,
            _glade->GetWidget ("timeline_viewport"));

      JobBoard::SetTimeLine (_timeline,
                             this);
    }

    {
      GList *warnings = Affinities::GetTitles ();

      while (warnings)
      {
        SetWarningColors ((const gchar *) warnings->data);

        warnings = g_list_next (warnings);
      }
    }

    {
      gchar *json = g_strdup_printf ("%s/BellePoule2D/hall.json", g_get_user_config_dir ());

      _json_file = new JsonFile (this,
                                 json);
      g_free (json);
    }

    SetToolBarSensitivity ();

    OnTimelineCursorMoved ();
  }

  // --------------------------------------------------------------------------------
  Hall::~Hall ()
  {
    _json_file->Save ();

    StopRedraw ();

    JobBoard::SetTimeLine (NULL,
                           NULL);

    FreeFullGList (Competition, _competition_list);
    FreeFullGList (Piste, _piste_list);

    _timeline->Release ();
    _lasso->Release    ();
    _clock->Release    ();
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
      _referee_key = _dnd_config->AddTarget ("bellepoule/referee", GTK_TARGET_SAME_APP|GTK_TARGET_OTHER_WIDGET);
      _job_key     = _dnd_config->AddTarget ("bellepoule/job",     GTK_TARGET_SAME_APP|GTK_TARGET_OTHER_WIDGET);

      _dnd_config->SetOnAWidgetDest (GTK_WIDGET (GetCanvas ()),
                                     GDK_ACTION_COPY);

      ConnectDndDest (GTK_WIDGET (GetCanvas ()));
      EnableDndOnCanvas ();
    }

    if (_json_file->Load () == FALSE)
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
  void Hall::SetWarningColors (const gchar *warning)
  {
    gchar     *box_name = g_strdup_printf ("%s_box", warning);
    GtkWidget *box      = _glade->GetWidget (box_name);

    if (box)
    {
      GdkColor  *gdk_color = Affinities::GetColor (warning);

      gtk_widget_modify_bg (box,
                            GTK_STATE_NORMAL,
                            gdk_color);
      gtk_widget_modify_bg (box,
                            GTK_STATE_ACTIVE,
                            gdk_color);
    }
    g_free (box_name);
  }

  // --------------------------------------------------------------------------------
  gboolean Hall::ReadJson (JsonReader *reader)
  {
    gboolean result = FALSE;

    if (json_reader_read_member (reader, "pistes"))
    {
      GDateTime *cursor = _timeline->RetreiveCursorTime (TRUE);
      guint      count  = json_reader_count_elements (reader);

      for (guint i = 0; i < count; i++)
      {
        Piste *piste  = new Piste (_clock, GetRootItem (), this, this);

        json_reader_read_element (reader, i);
        piste->ReadJson (reader);
        json_reader_end_element (reader);

        _drop_zones = g_slist_append (_drop_zones,
                                      piste);
        _piste_list = g_list_insert_sorted (_piste_list,
                                            piste,
                                            GCompareFunc (Piste::CompareId));

        piste->DisplayAtTime (cursor);
      }

      g_date_time_unref (cursor);
      result = TRUE;
    }

    json_reader_end_member (reader);

    return result;
  }

  // --------------------------------------------------------------------------------
  void Hall::FeedJsonBuilder (JsonBuilder *builder)
  {
    json_builder_set_member_name (builder, "pistes");
    json_builder_begin_array     (builder);

    {
      GList *current = _piste_list;

      while (current)
      {
        Piste *piste = (Piste *) current->data;

        piste->FeedJsonBuilder (builder);

        current = g_list_next (current);
      }
    }

    json_builder_end_array  (builder);
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
    if (key == _referee_key)
    {
      return _referee_pool->GetReferee (ref);
    }
    else if (key == _job_key)
    {
      GList *current_competition = _competition_list;

      while (current_competition)
      {
        Competition *competition   = (Competition *) current_competition->data;
        GList       *current_batch = competition->GetBatches ();

        while (current_batch)
        {
          Batch *batch = (Batch *) current_batch->data;
          Job   *job   = batch->GetJob (ref);

          if (job)
          {
            return job;
          }
          current_batch = g_list_next (current_batch);
        }

        current_competition = g_list_next (current_competition);
      }
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
      Job *job = dynamic_cast <Job *> (object);

      if (job)
      {
        GDateTime *from     = _timeline->RetreiveCursorTime ();
        GTimeSpan  duration = job->GetRegularDuration ();
        GList     *slots    = piste->GetFreeSlots (from, duration);
        Slot      *slot     = (Slot *) slots->data;

        slot->SetDuration (duration);
        slot->AddJob      (job);

        g_date_time_unref (from);

        Redraw ();
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
        Redraw ();
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Hall::ManageCompetition (Net::Message *message,
                                GtkNotebook  *notebook)
  {
    guint        id          = message->GetNetID ();
    Competition *competition = GetCompetition (id);

    if (competition == NULL)
    {
      competition = new Competition (id,
                                     this);

      _competition_list = g_list_prepend (_competition_list,
                                          competition);

      competition->SetProperties (message);
      competition->AttachTo (notebook);
      _timeline->AddCompetition (competition);
    }
    else
    {
      competition->SetProperties (message);
    }

    _referee_pool->Spread ();
  }

  // --------------------------------------------------------------------------------
  void Hall::DeleteCompetition (Net::Message *message)
  {
    guint        id          = message->GetNetID ();
    Competition *competition = GetCompetition (id);

    if (competition && (competition->GetBatches () == NULL))
    {
      GList *node = g_list_find (_competition_list,
                                 competition);

      _competition_list = g_list_delete_link (_competition_list,
                                              node);

      _timeline->RemoveCompetition (competition);
      competition->Release ();
    }
  }

  // --------------------------------------------------------------------------------
  void Hall::ManageBatch (Net::Message *message)
  {
    guint        id          = message->GetInteger ("competition");
    Competition *competition = GetCompetition (id);

    if (competition)
    {
      Batch     *batch      = competition->ManageBatch (message);
      DndConfig *dnd_config = batch->GetDndConfig ();

      dnd_config->SetPeerListener (this);
    }
  }

  // --------------------------------------------------------------------------------
  void Hall::DeleteBatch (Net::Message *message)
  {
    guint        id          = message->GetInteger ("competition");
    Competition *competition = GetCompetition (id);

    if (competition)
    {
      competition->DeleteBatch (message);
    }
  }

  // --------------------------------------------------------------------------------
  void Hall::ManageJob (Net::Message *message)
  {
    guint        id          = message->GetInteger ("competition");
    Competition *competition = GetCompetition (id);

    if (competition)
    {
      Batch *batch = competition->GetBatch (message->GetInteger ("batch"));

      if (batch)
      {
        GTimeSpan  real_duration = message->GetInteger ("duration_sec") * G_TIME_SPAN_SECOND;
        Job       *job           = batch->GetJob (message->GetNetID ());

        if (job)
        {
          job->SetRealDuration (real_duration);
          Redraw ();
        }
        else
        {
          guint    piste_id;
          guint    referee_id;
          FieTime *start_time = NULL;

          job = batch->Load (message,
                             &piste_id,
                             &referee_id,
                             &start_time);
          if (job)
          {
            job->SetWeapon (Weapon::GetFromXml (competition->GetWeaponCode ()));

            if (start_time)
            {
              Piste *piste = GetPiste (piste_id);

              if (piste)
              {
                GTimeSpan  duration = job->GetRegularDuration ();
                Slot      *slot     = piste->GetFreeSlot (start_time->GetGDateTime (),
                                                          duration);

                if (slot)
                {
                  EnlistedReferee *referee = _referee_pool->GetReferee (referee_id);

                  if (referee)
                  {
                    slot->SetDuration (duration);
                    slot->AddJob      (job);
                    slot->AddReferee  (referee);

                    job->SetRealDuration (real_duration);

                    Redraw ();
                  }
                  else
                  {
                    slot->Release ();
                  }
                }
              }
              start_time->Release ();
            }
          }
          batch->CloseLoading ();
          _referee_pool->RefreshWorkload (competition->GetWeaponCode ());
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Hall::ManageFencer (Net::Message *message)
  {
    guint        id          = message->GetInteger ("competition");
    Competition *competition = GetCompetition (id);

    if (competition)
    {
      competition->ManageFencer (message);
    }
  }

  // --------------------------------------------------------------------------------
  void Hall::DeleteJob (Net::Message *message)
  {
    guint        id          = message->GetInteger ("competition");
    Competition *competition = GetCompetition (id);

    if (competition)
    {
      Batch *batch = competition->GetBatch (message->GetInteger ("batch"));

      if (batch)
      {
        batch->RemoveJob (message);
        _referee_pool->RefreshWorkload (competition->GetWeaponCode ());
        Redraw ();
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Hall::DeleteFencer (Net::Message *message)
  {
    guint        id          = message->GetInteger ("competition");
    Competition *competition = GetCompetition (id);

    if (competition)
    {
      competition->DeleteFencer (message);
    }
  }

  // --------------------------------------------------------------------------------
  Piste *Hall::AddPiste (gdouble tx, gdouble ty)
  {
    GList *anchor = g_list_last (_piste_list);
    Piste *piste  = new Piste (_clock, GetRootItem (), this, this);

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
      GDateTime *cursor = _timeline->RetreiveCursorTime (TRUE);

      piste->DisplayAtTime (cursor);
      g_date_time_unref (cursor);
    }

    return piste;
  }

  // --------------------------------------------------------------------------------
  Piste *Hall::GetPiste (guint id)
  {
    GList *current = _piste_list;

    while (current)
    {
      Piste *piste = (Piste *) current->data;

      if (piste->GetId () == id)
      {
        return piste;
      }

      current = g_list_next (current);
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  Competition *Hall::GetCompetition (guint id)
  {
    GList *current = _competition_list;

    while (current)
    {
      Competition *competition = (Competition *) current->data;

      if (competition->GetId () == id)
      {
        return competition;
      }

      current = g_list_next (current);
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  void Hall::RemoveSelected ()
  {
    GList *current = _selected_piste_list;

    while (current)
    {
      Piste *piste = (Piste *) current->data;

      RemovePiste (piste);

      current = g_list_next (current);
    }

    g_list_free (_selected_piste_list);
    _selected_piste_list = NULL;

    Redraw ();
  }

  // --------------------------------------------------------------------------------
  void Hall::AlignSelectedOnGrid ()
  {
    GList *current = _selected_piste_list;

    while (current)
    {
      Piste *piste = (Piste *) current->data;

      piste->AlignOnGrid ();

      current = g_list_next (current);
    }
  }

  // --------------------------------------------------------------------------------
  void Hall::AlterCurrentTime ()
  {
#if defined(DEBUG)
    GDateTime *cursor = _timeline->RetreiveCursorTime (TRUE);

    _clock->Set (cursor);

    g_date_time_unref (cursor);

    Redraw ();
#endif
  }

  // --------------------------------------------------------------------------------
  void Hall::TranslateSelected (gdouble tx,
                                gdouble ty)
  {
    GList *current = _selected_piste_list;

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
    GList *current = _selected_piste_list;

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
  void Hall::StopMovingJob ()
  {
    if (_floating_job)
    {
      _floating_job->RemoveData (this,
                                 "floating_referees");
      _floating_job = NULL;

      _listener->OnUnBlur ();
      gtk_widget_set_sensitive (_glade->GetWidget ("toolbar"),
                                TRUE);
      OnTimelineCursorMoved ();
    }

    ScheduleRedraw ();
  }

  // --------------------------------------------------------------------------------
  gboolean Hall::OnButtonPress (GooCanvasItem  *item,
                                GooCanvasItem  *target,
                                GdkEventButton *event,
                                Hall           *hall)
  {
    hall->StopMovingJob ();

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
    GList *current = _selected_piste_list;

    while (current)
    {
      Piste *current_piste = (Piste *) current->data;

      current_piste->UnSelect ();

      current = g_list_next (current);
    }

    g_list_free (_selected_piste_list);
    _selected_piste_list = NULL;

    SetToolBarSensitivity ();
  }

  // --------------------------------------------------------------------------------
  void Hall::SetToolBarSensitivity ()
  {
    gtk_widget_set_sensitive (_glade->GetWidget ("remove_piste_button"),
                              _selected_piste_list != NULL);
    gtk_widget_set_sensitive (_glade->GetWidget ("rotate_piste_button"),
                              _selected_piste_list != NULL);
    gtk_widget_set_sensitive (_glade->GetWidget ("cone_togglebutton"),
                              FALSE);
    if (_selected_piste_list)
    {
      GList    *current = _selected_piste_list;
      gboolean  first   = FALSE;

      while (current)
      {
        Piste *piste = (Piste *) current->data;

        if (current == _selected_piste_list)
        {
          first = piste->IsBlocked ();
        }
        else
        {
          if (piste->IsBlocked () != first)
          {
            return;
          }
        }

        current = g_list_next (current);
      }

      {
        GtkWidget *cone = _glade->GetWidget ("cone_togglebutton");

        __gcc_extension__ g_signal_handlers_disconnect_by_func (G_OBJECT (cone),
                                                                (void *) OnConeToggled,
                                                                (Object *) this);
        gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (cone),
                                           first);
        g_signal_connect (G_OBJECT (cone), "toggled",
                          G_CALLBACK (OnConeToggled), this);
        gtk_widget_set_sensitive (cone,
                                  TRUE);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Hall::SelectPiste (Piste *piste)
  {
    _selected_piste_list = g_list_insert_sorted (_selected_piste_list,
                                                 piste,
                                                 GCompareFunc (Piste::CompareId));

    if (_floating_job && (piste->Blured () == FALSE))
    {
      {
        GTimeSpan  duration = _floating_job->GetRegularDuration ();
        GDateTime *cursor   = _timeline->RetreiveCursorTime ();
        GList     *current  = (GList *) _floating_job->GetPtrData (this, "floating_referees");
        Slot      *slot;

        slot = piste->GetFreeSlot (cursor,
                                   duration);

        if (slot)
        {
          slot->SetDuration (duration);
          slot->AddJob      (_floating_job);

          while (current)
          {
            EnlistedReferee *referee = (EnlistedReferee *) current->data;

            slot->AddReferee (referee);
            current = g_list_next (current);
          }
        }

        g_date_time_unref (cursor);
      }

      _timeline->Redraw ();
      StopMovingJob ();
    }

    piste->Select ();

    SetToolBarSensitivity ();
  }

  // --------------------------------------------------------------------------------
  void Hall::UnSelectPiste (Piste *piste)
  {
    GList *selected_node = g_list_find (_selected_piste_list, piste);

    _selected_piste_list = g_list_remove_link (_selected_piste_list,
                                               selected_node);
    piste->UnSelect ();

    SetToolBarSensitivity ();
  }

  // --------------------------------------------------------------------------------
  void Hall::OnPisteDirty ()
  {
    _json_file->MakeDirty ();
  }

  // --------------------------------------------------------------------------------
  void Hall::OnPisteButtonEvent (Piste          *piste,
                                 GdkEventButton *event)
  {
    if ((event == NULL) || (event->type != GDK_BUTTON_PRESS))
    {
      ResetCursor ();
      _lasso->Pull ();

      if (_dragging)
      {
        _dragging = FALSE;

        AlignSelectedOnGrid ();
      }
    }
    else
    {
      if (g_list_find (_selected_piste_list, piste))
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
  }

  // --------------------------------------------------------------------------------
  void Hall::OnPisteMotionEvent (Piste          *piste,
                                 GdkEventMotion *event)
  {
    if (_dragging)
    {
      SetCursor (GDK_FLEUR);

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

      if (_floating_job)
      {
        if (piste->Blured ())
        {
          SetCursor (GDK_X_CURSOR);
        }
        else
        {
          SetCursor (GDK_PLUS);
        }
      }
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
    else if (_floating_job)
    {
      SetCursor (GDK_PLUS);
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
  void Hall::RefreshBookDate ()
  {
    GDateTime *new_date    = _timeline->RetreiveCursorTime (TRUE);
    GtkLabel  *popup_clock = GTK_LABEL (_glade->GetWidget ("cursor_date"));
    gchar     *text;

    text = g_strdup_printf ("<span weight=\"bold\" background=\"black\" foreground=\"white\"> %02d:%02d </span>",
                            g_date_time_get_hour   (new_date),
                            g_date_time_get_minute (new_date));

    gtk_label_set_markup (popup_clock,
                          text);

    g_free (text);
    g_date_time_unref (new_date);
  }


  // --------------------------------------------------------------------------------
  gboolean Hall::OnBatchAssignmentRequest (Batch *batch)
  {
    Competition *competition = batch->GetCompetition ();
    gboolean     done        = FALSE;

    _listener->OnExposeWeapon (competition->GetWeaponCode ());

    if (_referee_pool->WeaponHasReferees (competition->GetWeaponCode ()) == FALSE)
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
      }

      {
        GDateTime *cursor = _timeline->RetreiveCursorTime (TRUE);

        _timeline->SetCursorTime (cursor);

        g_date_time_unref (cursor);
      }

      done = gtk_dialog_run (GTK_DIALOG (dialog)) == 0;
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
            job_list = batch->RetreivePendingJobs ();
          }
          else
          {
            job_list = batch->RetreiveCurrentSelection ();
          }
        }

        // All the referees?
        {
          People::RefereesList *checkin_list = _referee_pool->GetListOf (competition->GetWeaponCode ());
          GtkToggleButton      *all_referees = GTK_TOGGLE_BUTTON (_glade->GetWidget ("all_referees"));

          if (gtk_toggle_button_get_active (all_referees))
          {
            referee_list = g_list_copy (checkin_list->GetList ());
          }
          else
          {
            referee_list = checkin_list->RetreiveSelectedPlayers ();
          }
        }

        current_job = job_list;
        while (current_job)
        {
          GList *free_slots = NULL;
          Job   *job        = (Job *) current_job->data;
          Slot  *job_slot   = job->GetSlot ();

          if (job_slot)
          {
            job_slot->Retain ();
            free_slots = g_list_append (free_slots,
                                        job_slot);
          }
          else
          {
            free_slots = GetFreePisteSlots (job->GetRegularDuration ());
          }

          GList *current_slot = free_slots;

          while (current_slot)
          {
            Slot *slot = (Slot *) current_slot->data;

            if (SetFreeRefereeFor (referee_list,
                                   slot,
                                   job))
            {
              break;
            }

            current_slot = g_list_next (current_slot);
          }
          FreeFullGList (Slot, free_slots);

          current_job = g_list_next (current_job);
        }

        {
          Job  *first = (Job *) job_list->data;
          Slot *slot  = first->GetSlot ();

          if (slot)
          {
            GDateTime *start = slot->GetStartTime ();

            _timeline->SetCursorTime (start);
          }
        }

        g_list_free (job_list);
        g_list_free (referee_list);

        _referee_pool->RefreshWorkload (competition->GetWeaponCode ());
      }

      gtk_widget_hide (dialog);

      _referee_pool->RefreshAvailability (_timeline,
                                          _piste_list);
    }

    return done;
  }

  // --------------------------------------------------------------------------------
  GList *Hall::GetFreePisteSlots (GTimeSpan duration)
  {
    GDateTime *when       = _timeline->RetreiveCursorTime (TRUE);
    GList     *free_slots = NULL;
    GList     *current    = _piste_list;

    // All the pistes?
    {
      GtkToggleButton *all_pistes = GTK_TOGGLE_BUTTON (_glade->GetWidget ("all_pistes"));

      if (gtk_toggle_button_get_active (all_pistes) == FALSE)
      {
        current = _selected_piste_list;
      }
    }

    // Loop over the pistes
    while (current)
    {
      Piste *piste     = (Piste *) current->data;
      GList *new_slots = piste->GetFreeSlots (when,
                                              duration);
      free_slots = g_list_concat (free_slots,
                                  new_slots);

      current = g_list_next (current);
    }

    g_date_time_unref (when);

    // Sort the result
    free_slots = g_list_sort (free_slots,
                              (GCompareFunc) Slot::CompareAvailbility);

    return free_slots;
  }

  // --------------------------------------------------------------------------------
  gint Hall::CompareReferee (EnlistedReferee *a,
                             EnlistedReferee *b,
                             GList           *fencer_list)
  {
    gint result = EnlistedReferee::CompareWorkload (a, b);

    if (result == 0)
    {
      Affinities *a_affinities = (Affinities *) a->GetPtrData (NULL, "affinities");
      Affinities *b_affinities = (Affinities *) b->GetPtrData (NULL, "affinities");

      if (a_affinities && b_affinities)
      {
        guint  affinity_count = g_list_length (Affinities::GetTitles ());
        guint *a_kinship      = g_new0 (guint, affinity_count);
        guint *b_kinship      = g_new0 (guint, affinity_count);

        while (fencer_list)
        {
          Player     *fencer       = (Player *) fencer_list->data;
          Affinities *f_affinities = (Affinities *) fencer->GetPtrData (NULL, "affinities");

          for (guint i = 1; i <= affinity_count; i = i<<1)
          {
            guint kinship;

            kinship = a_affinities->KinshipWith (f_affinities);
            if (kinship & i)
            {
              a_kinship[i-1]++;
            }

            kinship = b_affinities->KinshipWith (f_affinities);
            if (kinship & i)
            {
              b_kinship[i-1]++;
            }
          }
          fencer_list = g_list_next (fencer_list);
        }

        for (guint i = 0; i < affinity_count; i++)
        {
          if (a_kinship[i] != b_kinship[i])
          {
            result = a_kinship[i] - b_kinship[i];
            break;
          }
        }

        g_free (a_kinship);
        g_free (b_kinship);
      }
    }

    return result;
  }

  // --------------------------------------------------------------------------------
  gboolean Hall::SetFreeRefereeFor (GList *referee_list,
                                    Slot  *slot,
                                    Job   *job)
  {
    GList     *current_referee;
    GList     *sorted_list;
    GTimeSpan  duration        = job->GetRegularDuration ();
    gboolean   result          = FALSE;

    sorted_list = g_list_sort_with_data (g_list_copy (referee_list),
                                         //(GCompareDataFunc) EnlistedReferee::CompareWorkload,
                                         (GCompareDataFunc) CompareReferee,
                                         job->GetFencerList ());

    current_referee = sorted_list;
    while (current_referee)
    {
      EnlistedReferee *referee      = (EnlistedReferee *) current_referee->data;
      Slot            *referee_slot = referee->GetAvailableSlotFor (slot,
                                                                    duration);

      if (referee_slot)
      {
        result = TRUE;

        slot->Retain ();
        slot->TailWith (referee_slot,
                        duration);
        slot->AddJob     (job);
        slot->AddReferee (referee);

        referee_slot->Release ();
        break;
      }

      current_referee = g_list_next (current_referee);
    }

    g_list_free (sorted_list);
    return result;
  }

  // --------------------------------------------------------------------------------
  void Hall::OnTimelineCursorChanged ()
  {
    _referee_pool->RefreshAvailability (_timeline,
                                        _piste_list);

    RefreshBookDate ();
  }

  // --------------------------------------------------------------------------------
  void Hall::OnTimelineCursorMoved ()
  {
    GDateTime *cursor        = _timeline->RetreiveCursorTime ();
    GList     *current_piste = _piste_list;

    while (current_piste)
    {
      Piste *piste = (Piste *) current_piste->data;

      piste->DisplayAtTime (cursor);

      if (_floating_job)
      {
        if (piste->GetSlotAt (cursor))
        {
          piste->Blur ();
        }
        else
        {
          GList     *current_referee = (GList *) _floating_job->GetPtrData (this, "floating_referees");
          GTimeSpan  duration        = _floating_job->GetRegularDuration ();
          Slot      *dest_slot       = piste->GetFreeSlot (cursor, duration);

          if (dest_slot == FALSE)
          {
            piste->Blur ();
          }
          else
          {
            while (current_referee)
            {
              EnlistedReferee *referee = (EnlistedReferee *) current_referee->data;

              if (referee->IsAvailableFor (dest_slot,
                                           duration) == FALSE)
              {
                piste->Blur ();
                break;
              }

              current_referee = g_list_next (current_referee);
            }
            dest_slot->Release ();
          }
        }
      }

      current_piste = g_list_next (current_piste);
    }
    g_date_time_unref (cursor);
  }

  // --------------------------------------------------------------------------------
  void Hall::OnJobBoardFocus (guint focus)
  {
    Piste *piste = GetPiste (focus);

    CancelSelection ();
    SelectPiste (piste);
  }

  // --------------------------------------------------------------------------------
  void Hall::OnJobMove (Job *job)
  {
    CancelSelection ();

    _floating_job = job;

    {
      Slot  *slot  = _floating_job->GetSlot ();
      Batch *batch = _floating_job->GetBatch ();

      _floating_job->SetData (this,
                              "floating_referees",
                              g_list_copy (slot->GetRefereeList ()),
                              GDestroyNotify (g_list_free));

      slot->RemoveJob (_floating_job);
      OnJobBoardUpdated (batch->GetCompetition ());
    }

    gtk_widget_set_sensitive (_glade->GetWidget ("toolbar"),
                              FALSE);
    StopRedraw ();
    _listener->OnBlur ();

    OnTimelineCursorMoved ();
  }

  // --------------------------------------------------------------------------------
  void Hall::OnJobBoardUpdated (Competition *competition)
  {
    Redraw ();
    _referee_pool->RefreshWorkload (competition->GetWeaponCode ());
  }

  // --------------------------------------------------------------------------------
  void Hall::OnBatchAssignmentCancel (Batch *batch)
  {
    Competition *competition   = batch->GetCompetition ();
    GList       *current_piste = _piste_list;

    while (current_piste)
    {
      Piste *piste = (Piste *) current_piste->data;

      piste->RemoveBatch (batch);

      current_piste = g_list_next (current_piste);
    }

    Redraw ();
    _referee_pool->RefreshWorkload (competition->GetWeaponCode ());

    _listener->OnExposeWeapon (competition->GetWeaponCode ());
  }

  // --------------------------------------------------------------------------------
  gboolean Hall::DroppingIsAllowed (Object   *floating_object,
                                    DropZone *in_zone)
  {
    if (CanvasModule::DroppingIsAllowed (floating_object,
                                         in_zone))
    {
      Piste *piste = dynamic_cast <Piste *> (in_zone);

      return DroppingIsAllowed (floating_object,
                                piste);
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  gboolean Hall::DroppingIsAllowed (Object *floating_object,
                                    Piste  *piste)
  {
    EnlistedReferee *referee = dynamic_cast <EnlistedReferee *> (floating_object);
    Job             *job     = dynamic_cast <Job *> (floating_object);
    GDateTime       *cursor  = _timeline->RetreiveCursorTime ();
    Slot            *slot    = NULL;

    if (referee)
    {
      slot = piste->GetSlotAt (cursor);
    }
    else if (job && (job->GetSlot () == FALSE))
    {
      slot = piste->GetFreeSlot (cursor,
                                 job->GetRegularDuration ());
    }

    g_date_time_unref (cursor);

    if (slot)
    {
      if (referee)
      {
        return referee->IsAvailableFor (slot,
                                        slot->GetDuration ());
      }
      else if (job)
      {
        slot->Release ();
        return TRUE;
      }
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void Hall::OnConeToggled (GtkToggleToolButton *widget,
                            Hall                *hall)
  {
    GList *current = hall->_selected_piste_list;

    while (current)
    {
      Piste *piste = (Piste *) current->data;

      piste->Block (gtk_toggle_tool_button_get_active (widget));

      current = g_list_next (current);
    }
  }

  // --------------------------------------------------------------------------------
  void Hall::ScheduleRedraw ()
  {
    StopRedraw ();

    _redraw_timeout = g_timeout_add_seconds (60,
                                             (GSourceFunc) RedrawCbk,
                                             this);
  }

  // --------------------------------------------------------------------------------
  void Hall::StopRedraw ()
  {
    if (_redraw_timeout > 0)
    {
      g_source_remove (_redraw_timeout);
      _redraw_timeout = 0;
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Hall::Redraw ()
  {
    _timeline->Redraw ();

    {
      GList *current = _piste_list;

      while (current)
      {
        Piste *piste = (Piste *) current->data;

        piste->Redraw ();

        current = g_list_next (current);
      }
    }

    ScheduleRedraw ();
    _referee_pool->RefreshAvailability (_timeline,
                                        _piste_list);

    return G_SOURCE_CONTINUE;
  }

  // --------------------------------------------------------------------------------
  gboolean Hall::RedrawCbk (Hall *hall)
  {
    return hall->Redraw ();
  }

  // --------------------------------------------------------------------------------
  void Hall::OnDragAndDropEnd ()
  {
    OnTimelineCursorMoved ();
  }

  // --------------------------------------------------------------------------------
  void Hall::OnDragDataReceived (GtkWidget        *widget,
                                 GdkDragContext   *drag_context,
                                 gint              x,
                                 gint              y,
                                 GtkSelectionData *data,
                                 guint             key,
                                 guint             time)
  {
    CanvasModule::OnDragDataReceived (widget,
                                      drag_context,
                                      x,
                                      y,
                                      data,
                                      key,
                                      time);

    {
      GList *current = _piste_list;

      while (current)
      {
        Piste *piste = (Piste *) current->data;

        if (DroppingIsAllowed (_dnd_config->GetFloatingObject (),
                               piste) == FALSE)
        {
          piste->Blur ();
        }

        current = g_list_next (current);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Hall::OnNewWarningPolicy ()
  {
    {
      GtkToggleButton *toggle;

      toggle = GTK_TOGGLE_BUTTON (_glade->GetWidget ("club_checkbutton"));
      Affinities::SetValidity ("club", gtk_toggle_button_get_active (toggle));

      toggle = GTK_TOGGLE_BUTTON (_glade->GetWidget ("league_checkbutton"));
      Affinities::SetValidity ("league", gtk_toggle_button_get_active (toggle));

      toggle = GTK_TOGGLE_BUTTON (_glade->GetWidget ("country_checkbutton"));
      Affinities::SetValidity ("country", gtk_toggle_button_get_active (toggle));
    }

    {
      GList *current_competition = _competition_list;

      while (current_competition)
      {
        Competition *competition   = (Competition *) current_competition->data;
        GList       *current_batch = competition->GetBatches ();

        while (current_batch)
        {
          Batch *batch       = (Batch *) current_batch->data;
          GList *current_job = batch->GetScheduledJobs ();

          while (current_job)
          {
            Job *job = (Job *) current_job->data;

            job->RefreshStatus ();
            batch->OnNewAffinitiesRule (job);

            current_job = g_list_next (current_job);
          }

          current_batch = g_list_next (current_batch);
        }

        current_competition = g_list_next (current_competition);
      }
    }

    JobBoard::ForeRedraw ();
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
  extern "C" G_MODULE_EXPORT void on_clock_button_clicked (GtkButton *widget,
                                                           Object    *owner)
  {
    Hall *h = dynamic_cast <Hall *> (owner);

    h->AlterCurrentTime ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_warning_checkbutton_toggled (GtkToggleButton *togglebutton,
                                                                  Object          *owner)
  {
    Hall *h = dynamic_cast <Hall *> (owner);

    h->OnNewWarningPolicy ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT gchar *on_timeline_format_value (GtkScale *scale,
                                                              gdouble   value)
  {
    return g_strdup_printf ("T0+%02d\'", (guint) value);
  }
}
