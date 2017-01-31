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
#include "slot.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  Slot::Slot (Owner     *owner,
              GDateTime *start_time,
              GDateTime *end_time,
              GTimeSpan  duration)
    : Object ("Slot")
  {
    _job_list     = NULL;
    _referee_list = NULL;
    _owner        = owner;

    _fie_time = new FieTime (start_time);
    _duration = duration;

    _start = start_time;
    g_date_time_ref (_start);
    if (_start)
    {
      g_date_time_ref (_start);
    }

    _end = end_time;
    if (_end)
    {
      g_date_time_ref (_end);
    }
  }

  // --------------------------------------------------------------------------------
  Slot::~Slot ()
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

    if (_start)
    {
      g_date_time_unref (_start);
    }

    if (_end)
    {
      g_date_time_unref (_end);
    }
  }

  // --------------------------------------------------------------------------------
  GDateTime *Slot::GetStartTime ()
  {
    return _start;
  }

  // --------------------------------------------------------------------------------
  GDateTime *Slot::GetEndTime ()
  {
    return _end;
  }

  // --------------------------------------------------------------------------------
  gboolean Slot::FitWith (Slot *with)
  {
    GDateTime *fixed_end;
    GTimeSpan  delta = g_date_time_difference (with->_start,
                                               _start);

    if (delta <= 0)
    {
      delta = 0;
    }
    fixed_end = g_date_time_add (_start, _duration + delta);

    if (_end && (g_date_time_compare (fixed_end, _end) > 0))
    {
      g_date_time_unref (fixed_end);
      return FALSE;
    }
    else if (with->_end && (g_date_time_compare (fixed_end, with->_end) > 0))
    {
      g_date_time_unref (fixed_end);
      return FALSE;
    }
    else
    {
      GDateTime *fixed_start = g_date_time_add (_start, delta);

      g_date_time_unref (_start);
      _start = fixed_start;

      if (_end)
      {
        g_date_time_unref (_end);
      }
      _end = fixed_end;
    }

    return TRUE;
  }

  // --------------------------------------------------------------------------------
  GTimeSpan Slot::GetDuration ()
  {
    return _duration;
  }

  // --------------------------------------------------------------------------------
  void Slot::Cancel ()
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
  void Slot::AddJob (Job *job)
  {
    _job_list = g_list_append (_job_list,
                               job);

    job->AddObjectListener (this);
    job->SetSlot           (this);

    {
      job->SetPiste (_owner->GetId (),
                     _fie_time->GetXmlImage ());

      if (_referee_list)
      {
        EnlistedReferee *referee = (EnlistedReferee *) _referee_list->data;

        job->SetReferee (referee->GetRef ());
      }
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
  void Slot::RemoveJob (Job *job)
  {
    {
      GList *node = g_list_find (_job_list,
                                 job);

      _job_list = g_list_delete_link (_job_list,
                                      node);
    }

    job->ResetRoadMap ();

    {
      Batch *batch = job->GetBatch ();

      batch->SetVisibility (job,
                            TRUE);;
    }

    job->RemoveObjectListener (this);
    job->SetSlot              (NULL);

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
  void Slot::AddReferee (EnlistedReferee *referee)
  {
    _referee_list = g_list_prepend (_referee_list,
                                    referee);

    referee->AddSlot (this);

    {
      GList *current = _job_list;

      while (current)
      {
        Job *job = (Job *) current->data;

        job->SetReferee (referee->GetRef ());

        current = g_list_next (current);
      }
    }

    _owner->OnSlotUpdated (this);
  }

  // --------------------------------------------------------------------------------
  void Slot::OnObjectDeleted (Object *object)
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
  GList *Slot::GetJobList ()
  {
    return _job_list;
  }

  // --------------------------------------------------------------------------------
  GList *Slot::GetRefereeList ()
  {
    return _referee_list;
  }

  // --------------------------------------------------------------------------------
  gint Slot::CompareAvailbility (Slot *a,
                                 Slot *b)
  {
    return g_date_time_compare (a->_start, b->_start);
  }

  // --------------------------------------------------------------------------------
  GDateTime *Slot::GetLatestDate (GDateTime *a,
                                  GDateTime *b)
  {
    if (g_date_time_compare (a, b) >= 0)
    {
      return a;
    }

    return b;
  }

  // --------------------------------------------------------------------------------
  GList *Slot::GetFreeSlots (Owner     *owner,
                             GList     *booked_slots,
                             GDateTime *from,
                             GTimeSpan  duration)
  {
    GList     *slots        = NULL;
    GList     *current      = booked_slots;
    GDateTime *rounded_from = GetRoundedDate (from);

    while (current)
    {
      GList *next         = g_list_next (current);
      Slot  *current_slot = (Slot *) current->data;
      Slot  *free_slot    = NULL;

      if (current == booked_slots)
      {
        // Before the booked slots
        GDateTime *end_time    = g_date_time_add (rounded_from, duration);
        gboolean   enough_time = (g_date_time_compare (current_slot->_start, end_time) >= 0);

        g_date_time_unref (end_time);
        if (enough_time)
        {
          break;
        }
      }
      if (next == NULL)
      {
        // After the booked slots
        free_slot = new Slot (owner,
                              GetLatestDate (rounded_from, current_slot->_end),
                              NULL,
                              duration);
      }
      else
      {
        // Between the booked slots
        Slot      *next_slot      = (Slot *) next->data;
        GDateTime *max_start_time;

        if (duration == 0)
        {
          duration = g_date_time_difference (next_slot->_end,
                                             current_slot->_start);
        }

        max_start_time = g_date_time_add (next_slot->_start, -duration);

        if (   (g_date_time_compare (max_start_time, current_slot->_end) > 0)
            && (g_date_time_compare (max_start_time, rounded_from) >= 0))
        {
          free_slot = new Slot (owner,
                                GetLatestDate (rounded_from, current_slot->_end),
                                next_slot->_start,
                                duration);
        }
        g_date_time_unref (max_start_time);
      }

      if (free_slot)
      {
        slots = g_list_append (slots,
                               free_slot);
      }

      current = g_list_next (current);
    }

    if (slots == NULL)
    {
      GDateTime *now = g_date_time_new_now_local ();

      {
        GDateTime *rounded_date = Slot::GetRoundedDate (now);

        g_date_time_unref (now);
        now = rounded_date;
      }

      {
        Slot *free_slot = new Slot (owner,
                                    GetLatestDate (rounded_from, now),
                                    NULL,
                                    duration);
        slots = g_list_append (slots,
                               free_slot);
      }

      g_date_time_unref (now);
    }

    g_date_time_unref (rounded_from);

    return slots;
  }

  // --------------------------------------------------------------------------------
  GDateTime *Slot::GetRoundedDate (GDateTime *of)
  {
    GDateTime *tmp;
    GDateTime *new_time;

    new_time = g_date_time_add_minutes (of, 15);

    // Round minutes
    {
      gint minutes = g_date_time_get_minute (new_time);
      gint crumbs  = minutes % 15;

      if (crumbs > 15/2)
      {
        tmp = g_date_time_add_minutes (new_time, 15 - crumbs);
        g_date_time_unref (new_time);
        new_time = tmp;
      }
      else
      {
        tmp = g_date_time_add_minutes (new_time, -crumbs);
        g_date_time_unref (new_time);
        new_time = tmp;
      }
    }

    tmp = g_date_time_new_local (g_date_time_get_year         (new_time),
                                 g_date_time_get_month        (new_time),
                                 g_date_time_get_day_of_month (new_time),
                                 g_date_time_get_hour         (new_time),
                                 g_date_time_get_minute       (new_time),
                                 0);
    g_date_time_unref (new_time);
    new_time = tmp;

    return new_time;
  }

  // --------------------------------------------------------------------------------
  Slot::Owner *Slot::GetOwner ()
  {
    return _owner;
  }

  // --------------------------------------------------------------------------------
  void Slot::Dump (Slot *what)
  {
    printf ("%p ==> %s\n", (void *) what->_owner, g_date_time_format (what->_start, "%T"));
  }
}
