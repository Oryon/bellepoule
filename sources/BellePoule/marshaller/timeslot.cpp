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
#include "timeslot.hpp"

// --------------------------------------------------------------------------------
TimeSlot::TimeSlot (Owner     *owner,
                    GDateTime *start_time)
  : Object ("TimeSlot")
{
  _job_list     = NULL;
  _referee_list = NULL;
  _owner        = owner;
  _duration     = 0;

  _start_time = start_time;
  g_date_time_ref (_start_time);
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

  g_date_time_unref (_start_time);
}

// --------------------------------------------------------------------------------
GDateTime *TimeSlot::GetStartTime ()
{
  return _start_time;;
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
  _duration += 10*60*1000*1000;

  job->AddObjectListener (this);
  job->SetTimslot        (this);

  {
    Net::Message *roadmap = job->GetRoadMap ();

    roadmap->Set ("piste",
                  _owner->GetId ());

    if (_referee_list)
    {
      Referee *referee = (Referee *) _referee_list->data;
      roadmap->Set ("referee", referee->GetRef ());
    }

    job->Spread ();
  }

  {
    Batch *batch = job->GetBatch ();

    batch->SetVisibility (job,
                          FALSE);
  }

  _owner->OnTimeSlotUpdated (this);
}

// --------------------------------------------------------------------------------
void TimeSlot::RemoveJob (Job *job)
{
  _duration -= 10*60*1000*1000;

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
  job->SetTimslot           (NULL);

  _owner->OnTimeSlotUpdated (this);
}

// --------------------------------------------------------------------------------
void TimeSlot::AddReferee (Referee *referee)
{
  _referee_list = g_list_prepend (_referee_list,
                                  referee);

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

  _owner->OnTimeSlotUpdated (this);
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

    _owner->OnTimeSlotUpdated (this);
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
  GDateTime *time_a = g_date_time_add (a->_start_time, a->_duration);
  GDateTime *time_b = g_date_time_add (b->_start_time, b->_duration);
  gint       delta  = g_date_time_compare (time_a, time_b);

  g_date_time_unref (time_a);
  g_date_time_unref (time_b);

  return delta;
}
