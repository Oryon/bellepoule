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
#include "enlisted_referee.hpp"
#include "job.hpp"
#include "batch.hpp"
#include "timeslot.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  TimeSlot::TimeSlot (Owner     *owner,
                      GDateTime *start_time,
                      GTimeSpan  duration)
    : Object ("TimeSlot")
  {
    _job_list     = NULL;
    _referee_list = NULL;
    _owner        = owner;

    _fie_time = new FieTime (start_time);
    _duration = duration;

    _start = start_time;
    g_date_time_ref (_start);

    _end = g_date_time_add (start_time, duration);
  }

  // --------------------------------------------------------------------------------
  TimeSlot::~TimeSlot ()
  {
    {
      GList *current = _job_list;

      while (current)
      {
        Job   *job   = (Job *) current->data;
        Batch *batch = job->GetBatch ();

        batch->SetVisibility (job,
                              TRUE);
        job->RemoveObjectListener (this);

        current = g_list_next (current);
      }

      g_list_free (_job_list);
    }

    g_list_free (_referee_list);

    _fie_time->Release ();

    g_date_time_unref (_start);
    g_date_time_unref (_end);
  }

  // --------------------------------------------------------------------------------
  GDateTime *TimeSlot::GetStartTime ()
  {
    return _start;
  }

  // --------------------------------------------------------------------------------
  GTimeSpan TimeSlot::GetInterval (TimeSlot *with)
  {
    GTimeSpan  interval;
    GDateTime *end_time = g_date_time_add (_start, _duration);

    interval = g_date_time_difference (end_time,
                                       with->GetStartTime ());
    g_date_time_unref (end_time);

    return interval;
  }

  // --------------------------------------------------------------------------------
  gboolean TimeSlot::Overlaps (TimeSlot *what)
  {
    if (g_date_time_compare (_start, what->_start) >= 0)
    {
      return (g_date_time_compare (_start, what->_end) <= 0);
    }
    else
    {
      return (g_date_time_compare (what->_start, _end) <= 0);
    }
  }

  // --------------------------------------------------------------------------------
  GTimeSpan TimeSlot::GetDuration ()
  {
    return _duration;
  }

  // --------------------------------------------------------------------------------
  void TimeSlot::Cancel ()
  {
    GList *job_list = g_list_copy (_job_list);
    GList *current  = job_list;

    while (current)
    {
      Job *job = (Job *) current->data;

      RemoveJob (job);

      current = g_list_next (current);
    }

    g_list_free (job_list);
  }

  // --------------------------------------------------------------------------------
  void TimeSlot::AddJob (Job *job)
  {
    _job_list = g_list_append (_job_list,
                               job);

    job->AddObjectListener (this);
    job->SetTimeSlot       (this);

    {
      Net::Message *roadmap = job->GetRoadMap ();

      roadmap->Set ("piste",
                    _owner->GetId ());

      roadmap->Set ("start_time",
                    _fie_time->GetXmlImage ());

      if (_referee_list)
      {
        EnlistedReferee *referee = (EnlistedReferee *) _referee_list->data;

        roadmap->Set ("referee", referee->GetRef ());
      }

      job->Spread ();
    }

    {
      Batch *batch = job->GetBatch ();

      batch->SetVisibility (job,
                            FALSE);
    }

    _owner->OnSlotUpdated (this);
    _owner->OnSlotLocked  (this);
  }

  // --------------------------------------------------------------------------------
  void TimeSlot::RemoveJob (Job *job)
  {
    {
      GList *node = g_list_find (_job_list,
                                 job);

      _job_list = g_list_delete_link (_job_list,
                                      node);
    }

    {
      Net::Message *roadmap = job->GetRoadMap ();

      roadmap->Recall ();
    }

    {
      Batch *batch = job->GetBatch ();

      batch->SetVisibility (job,
                            TRUE);;
    }

    job->RemoveObjectListener (this);
    job->SetTimeSlot          (NULL);

    {
      g_list_foreach (_referee_list,
                      (GFunc) EnlistedReferee::OnRemovedFromSlot,
                      this);
      g_list_free (_referee_list);
      _referee_list = NULL;
    }

    _owner->OnSlotUpdated (this);

    _duration = 0;
  }

  // --------------------------------------------------------------------------------
  void TimeSlot::AddReferee (EnlistedReferee *referee)
  {
    _referee_list = g_list_prepend (_referee_list,
                                    referee);

    referee->AddSlot (this);

    {
      GList *current = _job_list;

      while (current)
      {
        Job          *job     = (Job *) current->data;
        Net::Message *roadmap = job->GetRoadMap ();

        roadmap->Set ("referee", referee->GetRef ());
        job->Spread ();

        current = g_list_next (current);
      }
    }

    _owner->OnSlotUpdated (this);
  }

  // --------------------------------------------------------------------------------
  void TimeSlot::OnObjectDeleted (Object *object)
  {
    Job *job = (Job *) object;

    if (job)
    {
      GList *node = g_list_find (_job_list,
                                 job);

      if (node)
      {
        _job_list = g_list_delete_link (_job_list,
                                        node);
      }

      _owner->OnSlotUpdated (this);
    }
  }

  // --------------------------------------------------------------------------------
  GList *TimeSlot::GetJobList ()
  {
    return _job_list;
  }

  // --------------------------------------------------------------------------------
  GList *TimeSlot::GetRefereeList ()
  {
    return _referee_list;
  }

  // --------------------------------------------------------------------------------
  gint TimeSlot::CompareAvailbility (TimeSlot *a,
                                     TimeSlot *b)
  {
    gint delta = g_date_time_compare (a->_end, b->_end);

    return delta;
  }
}
