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

#include "network/message.hpp"
#include "slot.hpp"
#include "batch.hpp"
#include "job.hpp"
#include "job_details.hpp"
#include "timeline.hpp"
#include "competition.hpp"

#include "job_board.hpp"

namespace Marshaller
{
  Timeline           *JobBoard::_timeline = NULL;
  JobBoard::Listener *JobBoard::_listener = NULL;

  // --------------------------------------------------------------------------------
  JobBoard::JobBoard ()
    : Object ("JobBoard"),
      Module ("job_board.glade")
  {
    _dialog          = _glade->GetWidget ("dialog");
    _job_list        = NULL;
    _current_job     = NULL;
    _referee_details = NULL;
    _fencer_details  = NULL;

    _take_off_button = _glade->GetWidget ("take_off_button");
    _move_button     = _glade->GetWidget ("move_button");
  }

  // --------------------------------------------------------------------------------
  JobBoard::~JobBoard ()
  {
    Object::TryToRelease (_referee_details);
    Object::TryToRelease (_fencer_details);

    g_list_free (_job_list);
  }

  // --------------------------------------------------------------------------------
  void JobBoard::SetTimeLine (Timeline *timeline,
                              Listener *listener)
  {
    _timeline = timeline;
    _listener = listener;
  }

  // --------------------------------------------------------------------------------
  void JobBoard::OnObjectDeleted (Object *object)
  {
    Job   *job  = (Job *) object;
    GList *node = g_list_find (_job_list,
                               job);

    _job_list = g_list_delete_link (_job_list,
                                    node);

    if (_current_job == node)
    {
      gtk_dialog_response (GTK_DIALOG (_dialog),
                           0);
    }
  }

  // --------------------------------------------------------------------------------
  void JobBoard::ForgetCurrentJob ()
  {
    if (_current_job)
    {
      Job *job = (Job *) _current_job->data;

      job->SetListener (NULL);
    }
  }

  // --------------------------------------------------------------------------------
  void JobBoard::ForgetJob (Job      *job,
                            JobBoard *job_board)
  {
    job->RemoveObjectListener (job_board);

    if (job_board->_current_job && ((Job *) (job_board->_current_job->data) == job))
    {
      job_board->ForgetCurrentJob ();
    }
  }

  // --------------------------------------------------------------------------------
  void JobBoard::Clean ()
  {
    if (_job_list)
    {
      g_list_foreach (_job_list,
                      (GFunc) ForgetJob,
                      this);
      g_list_free (_job_list);
      _job_list = NULL;
    }

    _current_job = NULL;
  }

  // --------------------------------------------------------------------------------
  void JobBoard::AddJob (Job *job)
  {
    _job_list = g_list_append (_job_list,
                               job);
    job->AddObjectListener (this);
  }

  // --------------------------------------------------------------------------------
  void JobBoard::AddJobs (GList *jobs)
  {
    GList *current = jobs;

    while (current)
    {
      Job *job = (Job *) current->data;

      AddJob (job);
      current = g_list_next (current);
    }
  }

  // --------------------------------------------------------------------------------
  void JobBoard::Display (Job *job)
  {
    if (_timeline && _job_list)
    {
      _current_job = _job_list;

      if (_job_list && job)
      {
        _current_job = g_list_find (_job_list,
                                    job);
      }
      else
      {
        GList *current = _job_list;

        while (current)
        {
          GDateTime *cursor = _timeline->RetreiveCursorTime ();
          Slot      *slot;

          job  = (Job *) current->data;
          slot = job->GetSlot ();

          if (slot && (slot->TimeIsInside (cursor)))
          {
            _current_job = current;
            g_date_time_unref (cursor);
            break;
          }

          g_date_time_unref (cursor);
          current = g_list_next (current);
        }
      }

      DisplayCurrent ();

      {
        guint response = gtk_dialog_run (GTK_DIALOG (_dialog));

        if (_current_job)
        {
          Job *selected_job = (Job *) _current_job->data;

          if (response == 1)
          {
            Batch *batch = selected_job->GetBatch ();
            Slot  *slot  = selected_job->GetSlot ();

            slot->RemoveJob (selected_job);
            _listener->OnJobBoardUpdated (batch->GetCompetition ());
          }
          else if (response == 2)
          {
            _listener->OnJobMove (selected_job);
          }
        }
      }
      gtk_widget_hide (_dialog);
    }

    ForgetCurrentJob ();
  }

  // --------------------------------------------------------------------------------
  void JobBoard::SetSensitivity ()
  {
    if (_referee_details)
    {
      GtkWidget *take_off = _glade->GetWidget ("take_off_button");

      gtk_widget_set_sensitive (take_off, FALSE);
      _referee_details->SetPopupVisibility ("PlayersList::RemoveAction",
                                            FALSE);

      if (_current_job)
      {
        Job   *job   = (Job *) _current_job->data;
        Batch *batch = job->GetBatch ();

        gtk_widget_set_sensitive (take_off, batch->IsModifiable ());
        _referee_details->SetPopupVisibility ("PlayersList::RemoveAction",
                                              batch->IsModifiable ());
      }
    }

    if (_current_job)
    {
      Job   *job   = (Job *) _current_job->data;
      Batch *batch = job->GetBatch ();

      gtk_widget_set_sensitive (_move_button,
                                job->GetSlot () != NULL);
      gtk_widget_set_sensitive (_take_off_button,
                                batch->IsModifiable () && job->GetSlot ());
    }
  }

  // --------------------------------------------------------------------------------
  void JobBoard::OnJobUpdated (Job *job)
  {
    DisplayCurrent ();
  }

  // --------------------------------------------------------------------------------
  void JobBoard::DisplayCurrent ()
  {
    if (_timeline)
    {
      {
        {
          GtkLabel *title = GTK_LABEL (_glade->GetWidget ("current_job_label"));

          gtk_label_set_text (title, "");
        }

        if (_referee_details)
        {
          _referee_details->UnPlug ();
          _referee_details->Release ();
          _referee_details = NULL;
        }
        if (_fencer_details)
        {
          _fencer_details->UnPlug ();
          _fencer_details->Release ();
          _fencer_details = NULL;
        }
      }

      if (_current_job)
      {
        Job  *job  = (Job *) _current_job->data;
        Slot *slot = job->GetSlot ();

        job->SetListener (this);

        {
          GtkLabel *title_label = GTK_LABEL (_glade->GetWidget ("current_job_label"));
          GtkLabel *piste_label = GTK_LABEL (_glade->GetWidget ("piste_label"));
          GtkLabel *time_label  = GTK_LABEL (_glade->GetWidget ("time_label"));
          gchar    *time;
          gchar    *piste;

          gtk_label_set_text (title_label, job->GetName ());

          if (slot)
          {
            GDateTime *start_time = slot->GetStartTime ();
            guint      piste_id   = slot->GetOwner()->GetId ();

            _timeline->SetCursorTime (start_time);
            _listener->OnJobBoardFocus (piste_id);

            time  = g_date_time_format (start_time, "%H:%M");
            piste = g_strdup_printf    (gettext ("Piste %d"),
                                        piste_id);
          }
          else
          {
            time  = g_strdup (gettext ("No time!"));
            piste = g_strdup (gettext ("No piste!"));
          }

          gtk_label_set_text (piste_label, piste);
          gtk_label_set_text (time_label,  time);

          g_free (piste);
          g_free (time);
        }

        {
          if (slot)
          {
            _referee_details = new JobDetails (this,
                                               slot->GetRefereeList (),
                                               job,
                                               TRUE);
          }
          else
          {
            _referee_details = new JobDetails (this,
                                               NULL,
                                               job,
                                               TRUE);
          }

          Plug (_referee_details,
                _glade->GetWidget ("referee_detail_hook"));

          _referee_details->SetPopupVisibility ("PlayersList::ReadOnlyAction",
                                                FALSE);
          _referee_details->SetPopupVisibility ("PlayersList::WriteAction",
                                                FALSE);
        }

        {
          _fencer_details  = new JobDetails (this,
                                             job->GetFencerList (),
                                             job,
                                             FALSE);

          Plug (_fencer_details,
                _glade->GetWidget ("fencer_detail_hook"));
        }

        SetHeader ();
      }
    }

     SetSensitivity ();
  }

  // --------------------------------------------------------------------------------
  void JobBoard::SetProperty (GQuark    key_id,
                              gchar    *data,
                              JobBoard *job_board)
  {
    const gchar *property        = g_quark_to_string (key_id);
    gchar       *property_widget = g_strdup_printf ("%s_label", property);
    GtkLabel    *label           = GTK_LABEL (job_board->_glade->GetGObject (property_widget));

    gtk_label_set_text (label,
                        gettext (data));

    g_free (property_widget);
  }

  // --------------------------------------------------------------------------------
  void JobBoard::SetHeader ()
  {
    Job         *job         = (Job *) _current_job->data;
    Batch       *batch       = job->GetBatch ();
    Competition *competition = batch->GetCompetition ();
    GData       *properties  = competition->GetProperties ();

    // Title
    g_datalist_foreach (&properties,
                        (GDataForeachFunc) SetProperty,
                        this);

    // Color
    {
      GtkWidget   *header_box  = _glade->GetWidget ("header_box");

      gtk_widget_modify_bg (header_box,
                            GTK_STATE_NORMAL,
                            competition->GetColor ());
    }

    // Arrows
    {
      GtkWidget *arrow;

      arrow = _glade->GetWidget ("previous_job");
      gtk_widget_set_sensitive (arrow, g_list_previous (_current_job) != NULL);

      arrow = _glade->GetWidget ("next_job");
      gtk_widget_set_sensitive (arrow, g_list_next (_current_job) != NULL);
    }
  }

  // --------------------------------------------------------------------------------
  void JobBoard::OnPlayerRemoved (Player *player)
  {
    if (_current_job)
    {
      Job   *job   = (Job *) _current_job->data;
      Batch *batch = job->GetBatch ();
      Slot  *slot  = job->GetSlot ();

      slot->RemoveReferee ((EnlistedReferee *) player);
      _listener->OnJobBoardUpdated (batch->GetCompetition ());
    }
  }

  // --------------------------------------------------------------------------------
  void JobBoard::OnPreviousClicked ()
  {
    ForgetCurrentJob ();
    _current_job = g_list_previous (_current_job);
    DisplayCurrent ();
  }

  // --------------------------------------------------------------------------------
  void JobBoard::OnNextClicked ()
  {
    ForgetCurrentJob ();
    _current_job = g_list_next (_current_job);
    DisplayCurrent ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_previous_job_clicked (GtkWidget *widget,
                                                           Object    *owner)
  {
    JobBoard *jb = dynamic_cast <JobBoard *> (owner);

    jb->OnPreviousClicked ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_next_job_clicked (GtkWidget *widget,
                                                       Object    *owner)
  {
    JobBoard *jb = dynamic_cast <JobBoard *> (owner);

    jb->OnNextClicked ();
  }
}
