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
      _current_job = _job_list;
      DisplayCurrent ();
    }
  }

  // --------------------------------------------------------------------------------
  void JobBoard::ForgetJob (Job      *job,
                            JobBoard *job_board)
  {
    job->RemoveObjectListener (job_board);
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

    RunDialog (GTK_DIALOG (_dialog));
    gtk_widget_hide (_dialog);
  }

  // --------------------------------------------------------------------------------
  void JobBoard::DisplayCurrent ()
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

      if (slot)
      {
        _referee_details = new JobDetails (slot->GetRefereeList ());
      }
      else
      {
        _referee_details = new JobDetails (NULL);
      }

      Plug (_referee_details,
            _glade->GetWidget ("referee_detail_hook"));

      {
        _fencer_details  = new JobDetails (job->GetFencerList ());

        Plug (_fencer_details,
              _glade->GetWidget ("fencer_detail_hook"));
      }

      SetHeader ();
    }
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
    Job   *job        = (Job *) _current_job->data;
    Batch *batch      = job->GetBatch ();
    GData *properties = batch->GetProperties ();

    // Title
    g_datalist_foreach (&properties,
                        (GDataForeachFunc) SetProperty,
                        this);

    // Color
    {
      GtkWidget *header_box = _glade->GetWidget ("header_box");

      gtk_widget_modify_bg (header_box,
                            GTK_STATE_NORMAL,
                            batch->GetColor ());
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
  void JobBoard::OnPreviousClicked ()
  {
    _current_job = g_list_previous (_current_job);
    DisplayCurrent ();
  }

  // --------------------------------------------------------------------------------
  void JobBoard::OnNextClicked ()
  {
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
