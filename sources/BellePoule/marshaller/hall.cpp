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
#include "network/console/console_server.hpp"
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
#include "slot.hpp"

#include "hall.hpp"

namespace Marshaller
{
  enum class OverlapColumnId
  {
    ICON_id,
    MESSAGE_ptr,
    BATCH_ptr,
    COLOR_str
  };

  // --------------------------------------------------------------------------------
  Hall::Hall (RefereePool *referee_pool,
              Listener    *listener)
    : Object ("Hall"),
    CanvasModule ("hall.glade")
  {
    _listener            = listener;
    _piste_list          = nullptr;
    _selected_piste_list = nullptr;
    _redraw_timeout      = 0;
    _floating_job        = nullptr;

    _dragging = FALSE;

    SetZoomer (GTK_RANGE (_glade->GetWidget ("zoom_scale")));

    _competition_list = nullptr;

    _referee_pool = referee_pool;
    _referee_pool->SetDndPeerListener (this);

    _lasso = new Lasso ();
    _clock = new Clock (this);

    Net::ConsoleServer::ExposeVariable ("fix-affinities",  1);
    Net::ConsoleServer::ExposeVariable ("preserve-piste",  1);
    Net::ConsoleServer::ExposeVariable ("clock-alterable", 0);

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

    JobBoard::SetTimeLine (nullptr,
                           nullptr);

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

    return nullptr;
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

        slot->AddJob (job);

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
        gchar               *weapons     = weapon_attr->GetStrValue ();

        piste->AddReferee (referee);

        {
          gchar *weapon_code = g_new0 (gchar, 2);

          for (guint i = 0; weapons[i] != '\0'; i++)
          {
            weapon_code[0] = weapons[i];
            _referee_pool->RefreshWorkload (weapon_code);
          }
          g_free (weapon_code);
        }

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

    if (competition == nullptr)
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

    if (competition)
    {
      if (competition->GetBatches () == nullptr)
      {
        _competition_list = g_list_remove (_competition_list,
                                           competition);

        _timeline->RemoveCompetition (competition);
        competition->Release ();
      }
      else
      {
        competition->Freeze ();
      }
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
      Batch        *batch         = competition->GetBatch (message->GetNetID ());
      GtkListStore *overlap_store = GTK_LIST_STORE (_glade->GetGObject ("overlap_liststore"));
      GtkTreeIter   iter;
      gboolean      iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (overlap_store),
                                                                   &iter);

      while (iter_is_valid)
      {
        Batch *current_batch;

        gtk_tree_model_get (GTK_TREE_MODEL (overlap_store),
                            &iter,
                            OverlapColumnId::BATCH_ptr, &current_batch,
                            -1);

        if (current_batch == batch)
        {
          gtk_list_store_remove (overlap_store,
                                 &iter);
        }

        iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (overlap_store),
                                                  &iter);
      }

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
          GList   *referees;
          FieTime *start_time = nullptr;

          batch->Mute ();
          job = batch->Load (message,
                             &piste_id,
                             &referees,
                             &start_time);
          if (job)
          {
            job->SetWeapon (Weapon::GetFromXml (competition->GetWeaponCode ()));

            if (start_time)
            {
              Piste *piste = GetPiste (piste_id);

              if (piste == nullptr)
              {
                piste = AddPiste ();

                piste->SetId (piste_id);
              }

              if (piste)
              {
                GTimeSpan  duration = job->GetRegularDuration ();
                Slot      *slot     = piste->CreateSlot (start_time->GetGDateTime (),
                                                         duration);

                if (slot)
                {
                  slot->AddJob (job);

                  for (GList *current = referees; current; current = g_list_next (current))
                  {
                    EnlistedReferee *referee = _referee_pool->GetReferee (GPOINTER_TO_UINT (current->data));

                    if (referee)
                    {
                      slot->AddReferee (referee);
                    }
                  }

                  job->SetRealDuration (real_duration);
                  Redraw ();
                }
              }
              start_time->Release ();
            }
          }

          batch->UnMute ();
          g_list_free (referees);

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
        batch->Mute ();
        batch->RemoveJob (message);
        batch->UnMute ();

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

      for (guint i = 1; before != nullptr; i++)
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

    return nullptr;
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

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  void Hall::RemoveSelected ()
  {
    GList *removed = nullptr;

    for (GList *current = _selected_piste_list; current; current = g_list_next (current))
    {
      Piste *piste = (Piste *) current->data;

      if (piste->GetSlots () == nullptr)
      {
        removed = g_list_prepend (removed,
                                  current);
        RemovePiste (piste);
      }
    }

    for (GList *current = removed; current; current = g_list_next (current))
    {
      _selected_piste_list = g_list_delete_link (_selected_piste_list,
                                                 (GList *) current->data);
    }

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
  void Hall::AlterClock ()
  {
    if (Net::ConsoleServer::VariableIsSet ("clock-alterable"))
    {
      GDateTime *cursor = _timeline->RetreiveCursorTime (TRUE);

      _clock->Set (cursor);

      g_date_time_unref (cursor);

      Redraw ();
    }
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
        _drop_zones = g_slist_remove (_drop_zones,
                                      piste);

        _piste_list = g_list_delete_link (_piste_list,
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
      _floating_job = nullptr;

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
    for (GList *current = _selected_piste_list; current; current = g_list_next (current))
    {
      Piste *current_piste = (Piste *) current->data;

      current_piste->UnSelect ();
    }

    g_list_free (_selected_piste_list);
    _selected_piste_list = nullptr;

    SetToolBarSensitivity ();
  }

  // --------------------------------------------------------------------------------
  void Hall::SetToolBarSensitivity ()
  {
    gtk_widget_set_sensitive (_glade->GetWidget ("remove_piste_button"),
                              _selected_piste_list != nullptr);
    gtk_widget_set_sensitive (_glade->GetWidget ("rotate_piste_button"),
                              _selected_piste_list != nullptr);
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
          slot->AddJob (_floating_job);

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
    _selected_piste_list = g_list_remove (_selected_piste_list,
                                          piste);
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
    if ((event == nullptr) || (event->type != GDK_BUTTON_PRESS))
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

    if (_piste_list)
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
        GList *sticky_slots = nullptr;

        // All the jobs?
        {
          GtkToggleButton *all_jobs = GTK_TOGGLE_BUTTON (_glade->GetWidget ("all_jobs"));

          if (gtk_toggle_button_get_active (all_jobs))
          {
            job_list = batch->RetreivePendingJobs ();
          }
          else
          {
            job_list = batch->RetreivePendingSelected ();
          }
        }

        // All the referees?
        {
          People::RefereesList *checkin_list = _referee_pool->GetListOf (competition->GetWeaponCode ());
          const gchar          *weapon       = competition->GetWeaponCode ();
          GList                *raw_list;

          if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("all_referees"))))
          {
            raw_list = g_list_copy (checkin_list->GetList ());
          }
          else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("selected_referees"))))
          {
            raw_list = checkin_list->RetreiveSelectedPlayers ();
          }
          else
          {
            raw_list = nullptr;
          }

          referee_list = nullptr;
          for (GList *current = g_list_last (raw_list); current; current = g_list_previous (current))
          {
            EnlistedReferee     *referee = (EnlistedReferee *) current->data;
            Player::AttributeId  weapon_attr_id ("weapon");
            Attribute           *weapon_attr = referee->GetAttribute (&weapon_attr_id);

            if (weapon_attr && (g_strrstr (weapon_attr->GetStrValue (),
                                           weapon) != nullptr))
            {
              referee_list = g_list_prepend (referee_list,
                                             referee);
            }
          }

          g_list_free (raw_list);
        }

        batch->Mute ();

        for (GList *current = job_list; current; current = g_list_next (current))
        {
          GList *free_slots = nullptr;
          Job   *job        = (Job *) current->data;
          Slot  *job_slot   = job->GetSlot ();

          if (job_slot)
          {
            job_slot->Retain ();
            free_slots = g_list_append (free_slots,
                                        job_slot);
            sticky_slots = g_list_prepend (sticky_slots,
                                           job_slot);
          }
          else
          {
            free_slots = GetFreePisteSlots (job->GetRegularDuration ());
          }

          if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("no_referee"))))
          {
            Slot *slot = (Slot *) free_slots->data;

            slot->Retain ();
            slot->AddJob (job);
          }
          else
          {
            for (GList *current_slot = free_slots; current_slot; current_slot = g_list_next (current_slot))
            {
              Slot *slot = (Slot *) current_slot->data;

              if (SetFreeRefereeFor (referee_list,
                                     slot,
                                     job))
              {
                break;
              }
            }
          }

          FreeFullGList (Slot, free_slots);
        }

        FixAffinities (job_list);

        PreservePiste (job_list,
                       sticky_slots);

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
        g_list_free (sticky_slots);

        _referee_pool->RefreshWorkload (competition->GetWeaponCode ());
      }

      gtk_widget_hide (dialog);

      _referee_pool->RefreshAvailability (_timeline,
                                          _piste_list);

      batch->UnMute ();
      batch->Spread ();
    }

    return done;
  }

  // --------------------------------------------------------------------------------
  void Hall::FixAffinities (GList *jobs)
  {
    if (Net::ConsoleServer::VariableIsSet ("fix-affinities"))
    {
      for (GList *a = jobs; a; a = g_list_next (a))
      {
        Job  *job_a  = (Job *) a->data;
        Slot *slot_a = job_a->GetSlot ();

        if (slot_a)
        {
          GList *referees_a = slot_a->GetRefereeList ();

          if (g_list_length (referees_a) == 1)
          {
            EnlistedReferee *referee_a = (EnlistedReferee *) referees_a->data;

            for (GList *b = jobs; b; b = g_list_next (b))
            {
              if (a != b)
              {
                Job  *job_b  = (Job *) b->data;
                Slot *slot_b = job_b->GetSlot ();

                if (slot_a->Equals (slot_b))
                {
                  GList *referees_b = slot_b->GetRefereeList ();

                  if (g_list_length (referees_b) == 1)
                  {
                    EnlistedReferee *referee_b = (EnlistedReferee *) referees_b->data;
                    gint             delta_a   = job_a->GetKinship (referee_b) - job_a->GetKinship ();
                    gint             delta_b   = job_b->GetKinship (referee_a) - job_b->GetKinship ();

                    if ((delta_a + delta_b) < 0)
                    {
                      slot_a->RemoveReferee ((EnlistedReferee *) referee_a);
                      slot_b->RemoveReferee ((EnlistedReferee *) referee_b);

                      if (   referee_a->IsAvailableFor (slot_b, slot_b->GetDuration ())
                          && referee_b->IsAvailableFor (slot_a, slot_a->GetDuration ()))
                      {
                        slot_a->AddReferee (referee_b);
                        slot_b->AddReferee (referee_a);
                      }
                      else
                      {
                        slot_a->AddReferee (referee_a);
                        slot_b->AddReferee (referee_b);
                      }
                      break;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Hall::PreservePiste (GList *jobs,
                            GList *sticky_slots)
  {
    if (Net::ConsoleServer::VariableIsSet ("preserve-piste"))
    {
      GList *swap_list = nullptr;

      {
        Slot  *first_slot  = nullptr;
        GList *sorted_list = g_list_copy (jobs);

        sorted_list = g_list_sort (sorted_list,
                                   (GCompareFunc) Job::CompareStartTime);

        for (GList *current = sorted_list; current; current = g_list_next (current))
        {
          Job  *job      = (Job *) current->data;
          Slot *now_slot = job->GetSlot ();

          if (now_slot && now_slot->GetRefereeList ())
          {
            if (first_slot == nullptr)
            {
              first_slot = now_slot;
            }
            else if (g_list_find (sticky_slots,
                                  now_slot) == nullptr)
            {
              if (Slot::CompareAvailbility (first_slot, now_slot) != 0)
              {
                swap_list = g_list_append (swap_list,
                                           now_slot);
              }
            }
          }
        }

        g_list_free (sorted_list);
      }

      {
        GList *swappable_list = g_list_copy (swap_list);

        while (swap_list)
        {
          Slot            *slot_a        = (Slot *) swap_list->data;
          EnlistedReferee *referee_a     = (EnlistedReferee *) slot_a->GetRefereeList ()->data;
          Slot            *before_slot_a = referee_a->GetSlotJustBefore (slot_a);

          if (before_slot_a)
          {
            Piste *piste_needed_by_a = (Piste *) before_slot_a->GetPiste ();

            if (piste_needed_by_a != slot_a->GetPiste ())
            {
              Slot *free_slot = piste_needed_by_a->GetFreeSlot (slot_a->GetStartTime (),
                                                                slot_a->GetDuration ());

              if (free_slot)
              {
                slot_a->Swap (free_slot);
                slot_a->Release ();
              }
              else
              {
                for (GList *b = swappable_list; b; b = g_list_next (b))
                {
                  Slot *slot_b = (Slot *) b->data;

                  if (   (slot_b != slot_a)
                      && (slot_b->GetPiste () == piste_needed_by_a)
                      && slot_b->Equals (slot_a))
                  {
                    slot_a->Swap (slot_b);

                    slot_a = nullptr;
                    swap_list = g_list_remove (swap_list,
                                               slot_b);
                    swappable_list = g_list_remove (swappable_list,
                                                    slot_b);
                    break;
                  }
                }
              }
            }
          }

          swap_list = g_list_remove (swap_list,
                                     slot_a);
        }
        g_list_free (swappable_list);
      }

      g_list_free (swap_list);
    }
  }

  // --------------------------------------------------------------------------------
  GList *Hall::GetFreePisteSlots (GTimeSpan duration)
  {
    GDateTime *when       = _timeline->RetreiveCursorTime (TRUE);
    GList     *free_slots = nullptr;
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
    guint       result       = 0;
    Affinities *a_affinities = (Affinities *) a->GetPtrData (nullptr, "affinities");
    Affinities *b_affinities = (Affinities *) b->GetPtrData (nullptr, "affinities");

    if (a_affinities && b_affinities)
    {
      guint  affinity_count = g_list_length (Affinities::GetTitles ());
      guint *a_kinship      = g_new0 (guint, affinity_count);
      guint *b_kinship      = g_new0 (guint, affinity_count);

      while (fencer_list)
      {
        Player     *fencer       = (Player *) fencer_list->data;
        Affinities *f_affinities = (Affinities *) fencer->GetPtrData (nullptr, "affinities");

        for (guint i = 0; i < affinity_count; i++)
        {
          guint kinship;

          kinship = a_affinities->KinshipWith (f_affinities);
          if (kinship & 1<<i)
          {
            a_kinship[i]++;
          }

          kinship = b_affinities->KinshipWith (f_affinities);
          if (kinship & 1<<i)
          {
            b_kinship[i]++;
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

    if (result == 0)
    {
      result = EnlistedReferee::CompareWorkload (a, b);
    }

    return result;
  }

  // --------------------------------------------------------------------------------
  gboolean Hall::SetFreeRefereeFor (GList *referee_list,
                                    Slot  *slot,
                                    Job   *job)
  {
    GTimeSpan  duration    = job->GetRegularDuration ();
    gboolean   result      = FALSE;
    GList     *sorted_list;

    sorted_list = g_list_sort_with_data (g_list_copy (referee_list),
                                         (GCompareDataFunc) CompareReferee,
                                         job->GetFencerList ());

    for (GList *current_referee = sorted_list; current_referee; current_referee = g_list_next (current_referee))
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
    batch->Mute ();
    {
      Competition *competition = batch->GetCompetition ();

      for (GList *current_piste = _piste_list; current_piste; current_piste = g_list_next (current_piste))
      {
        Piste *piste = (Piste *) current_piste->data;

        piste->RemoveBatch (batch);
      }

      Redraw ();
      _referee_pool->RefreshWorkload (competition->GetWeaponCode ());

      _listener->OnExposeWeapon (competition->GetWeaponCode ());
    }
    batch->UnMute ();
    batch->Recall ();
  }

  // --------------------------------------------------------------------------------
  void Hall::OnJobOverlapWarning (Batch *batch)
  {
    GtkListStore *overlap_store = GTK_LIST_STORE (_glade->GetGObject ("overlap_liststore"));
    GtkTreeIter   iter;
    gboolean      iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (overlap_store),
                                                                 &iter);

    while (iter_is_valid)
    {
      Batch *current_batch;

      gtk_tree_model_get (GTK_TREE_MODEL (overlap_store),
                          &iter,
                          OverlapColumnId::BATCH_ptr, &current_batch,
                          -1);

      if (current_batch == batch)
      {
        break;
      }

      iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (overlap_store),
                                                &iter);
    }

    if (iter_is_valid == FALSE)
    {
      gtk_list_store_append (overlap_store,
                             &iter);
    }

    {
      Competition *competition = batch->GetCompetition ();
      gchar       *color       = gdk_color_to_string (competition->GetColor ());
      GData       *properties  = competition->GetProperties ();
      gchar       *name        = g_strdup_printf ("%s-%s-%s %s",
                                                  (gchar *) g_datalist_get_data (&properties, "gender"),
                                                  (gchar *) g_datalist_get_data (&properties, "weapon"),
                                                  (gchar *) g_datalist_get_data (&properties, "category"),
                                                  batch->GetName ());

      gtk_list_store_set (overlap_store, &iter,
                          OverlapColumnId::MESSAGE_ptr,   name,
                          OverlapColumnId::COLOR_str,     color,
                          OverlapColumnId::BATCH_ptr,     batch,
                          -1);

      g_free (name);
      g_free (color);
    }
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
    Slot            *slot    = nullptr;

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
        Job                 *piste_job   = (Job *) (slot->GetJobList ())->data;
        Batch               *batch       = piste_job->GetBatch ();
        Competition         *competition = batch->GetCompetition ();
        const gchar         *weapon      = competition->GetWeaponCode ();
        Player::AttributeId  weapon_attr_id ("weapon");
        Attribute           *weapon_attr = referee->GetAttribute (&weapon_attr_id);

        if (weapon_attr && (g_strrstr (weapon_attr->GetStrValue (),
                                       weapon) != nullptr))
        {
          return referee->IsAvailableFor (slot,
                                          slot->GetDuration ());
        }
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
  void Hall::OnOverlapWarningActivated (GtkTreePath *path)
  {
    GtkListStore *overlap_store = GTK_LIST_STORE (_glade->GetGObject ("overlap_liststore"));
    GtkTreeModel *model         = GTK_TREE_MODEL (overlap_store);
    gchar        *message;
    Batch        *batch;
    GtkTreeIter   iter;

    gtk_tree_model_get_iter (model,
                             &iter,
                             path);
    gtk_tree_model_get (model, &iter,
                        OverlapColumnId::MESSAGE_ptr, &message,
                        OverlapColumnId::BATCH_ptr,   &batch,
                        -1);

    {
      GtkWidget *error_dialog = _glade->GetWidget ("slot_overlap_dialog");

      gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (error_dialog),
                                                  gettext ("<b>%s</b> is delayed.\nDo you want to postpone the next bouts?\n"),
                                                  message);

      if (RunDialog (GTK_DIALOG (error_dialog)) == GTK_RESPONSE_YES)
      {
        batch->FixOverlapWarnings ();

        gtk_list_store_remove (overlap_store,
                               &iter);

        _timeline->Redraw ();
      }

      gtk_widget_hide (error_dialog);
    }

    g_free (message);
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

    h->AlterClock ();
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

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_overlap_treeview_row_activated (GtkTreeView       *tree_view,
                                                                     GtkTreePath       *path,
                                                                     GtkTreeViewColumn *column,
                                                                     Object            *owner)
  {
    Hall *h = dynamic_cast <Hall *> (owner);

    h->OnOverlapWarningActivated (path);
  }
}
