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
#include "piste.hpp"
#include "slot.hpp"
#include "resource.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  Slot::Slot (Piste     *piste,
              GDateTime *start_time,
              GDateTime *end_time,
              GTimeSpan  duration)
    : Object ("Slot")
  {
    _job_list     = nullptr;
    _referee_list = nullptr;
    _piste        = piste;

    _duration = duration;

    _fie_time = nullptr;
    _start    = nullptr;
    SetStartTime (start_time);

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
        Job *job = (Job *) current->data;

        RefreshJobStatus (job);
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
  gboolean Slot::IsOver ()
  {
    GList *current = _job_list;

    while (current)
    {
      Job *job = (Job *) current->data;

      if (job->IsOver () == FALSE)
      {
        return FALSE;
      }
      current = g_list_next (current);
    }

    return _job_list != nullptr;
  }

  // --------------------------------------------------------------------------------
  gboolean Slot::Equals (Slot *to)
  {
    if (g_date_time_compare (_start, to->_start) != 0)
    {
      return FALSE;
    }
    else if (_end && to->_end && g_date_time_compare (_end, to->_end) != 0)
    {
      return FALSE;
    }

    return TRUE;
  }

  // --------------------------------------------------------------------------------
  void Slot::Swap (Slot *with)
  {
    GList *job_list          = g_list_copy (_job_list);
    GList *referee_list      = g_list_copy (_referee_list);
    GList *with_job_list     = g_list_copy (with->_job_list);
    GList *with_referee_list = g_list_copy (with->_referee_list);

    Retain ();
    with->Retain ();

    Cancel       ();
    with->Cancel ();

    for (GList *current = job_list; current; current = g_list_next (current))
    {
      with->AddJob ((Job *) current->data);
    }
    for (GList *current = with_job_list; current; current = g_list_next (current))
    {
      AddJob ((Job *) current->data);
    }

    for (GList *current = referee_list; current; current = g_list_next (current))
    {
      with->AddReferee ((EnlistedReferee *) current->data);
    }
    for (GList *current = with_referee_list; current; current = g_list_next (current))
    {
      AddReferee ((EnlistedReferee *) current->data);
    }

    g_list_free (job_list);
    g_list_free (referee_list);
    g_list_free (with_job_list);
    g_list_free (with_referee_list);
  }

  // --------------------------------------------------------------------------------
  void Slot::SetStartTime (GDateTime *time)
  {
    if (_start)
    {
      g_date_time_unref (_start);
    }

    _start = time;
    if (_start)
    {
      g_date_time_ref (_start);
    }

    TryToRelease (_fie_time);
    _fie_time = new FieTime (time);
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
  void Slot::SetDuration (GTimeSpan duration,
                          gboolean  fix_overlaps)
  {
    if (IsOver () == FALSE)
    {
      {
        GDateTime *end = g_date_time_add (_start, duration);

        _duration = duration;
        if (_end)
        {
          g_date_time_unref (_end);
        }
        _end = end;
      }

      // FixOverlaps (!fix_overlaps);

      _piste->OnSlotUpdated (this);
    }
  }

  // --------------------------------------------------------------------------------
  void Slot::FixOverlaps (gboolean dry_run)
  {
    GList *resources = nullptr;

    if (_referee_list)
    {
      resources = g_list_prepend (resources, dynamic_cast <Resource *> ((EnlistedReferee *) _referee_list->data));
    }
    resources = g_list_prepend (resources, dynamic_cast <Resource *> (_piste));

    for (GList *current = resources; current != nullptr; current = g_list_next (current))
    {
      Resource *resource = (Resource *) current->data;
      Slot     *slot     = resource->GetSlotAfter (this);

      if (slot)
      {
        Job *dependent_job = (Job *) slot->_job_list->data;

        if (g_date_time_difference (_end, slot->_start) > 5*G_TIME_SPAN_MINUTE)
        {
          if (dry_run)
          {
            Job   *job   = (Job *) _job_list->data;
            Batch *batch = job->GetBatch ();

            batch->RaiseOverlapWarning ();
          }
          else
          {
            slot->SetStartTime (_end);

            slot->SetDuration (slot->_duration,
                               TRUE);

            dependent_job->SetPiste (_piste->GetId (),
                           slot->_fie_time->GetXmlImage ());
            dependent_job->Spread ();
          }
        }
      }
    }

    g_list_free (resources);
  }

  // --------------------------------------------------------------------------------
  void Slot::TailWith (Slot      *with,
                       GTimeSpan  duration)
  {
    g_warning ("**** JobDetails::OnDragDrop ****");
    SetDuration (duration);
  }

  // --------------------------------------------------------------------------------
  gboolean Slot::CanWrap (Slot *with)
  {
    gboolean wrap = TRUE;

    if (g_date_time_difference (with->_start,
                                _start) > 0)
    {
      wrap = FALSE;
    }
    else if (with->_end)
    {
      GDateTime *end = g_date_time_add (_start, _duration);

      if (g_date_time_compare (end, with->_end) > 0)
      {
        wrap = FALSE;
      }

      g_date_time_unref (end);
    }

    return wrap;
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

    for (GList *current = job_list; current; current = g_list_next (current))
    {
      Job *job = (Job *) current->data;

      RemoveJob (job);
      job->ResetRoadMap ();
    }

    g_list_free (job_list);
  }

  // --------------------------------------------------------------------------------
  void Slot::AddJob (Job *job)
  {
    if (g_list_find (_job_list,
                     job) == nullptr)
    {
      SetDuration (job->GetRegularDuration ());

      {
        gboolean assigned_to_piste = (_job_list != nullptr);

        _job_list = g_list_append (_job_list,
                                   job);

        job->AddObjectListener (this);
        job->SetSlot           (this);

        {
          job->SetPiste (_piste->GetId (),
                         _fie_time->GetXmlImage ());

          if (_referee_list)
          {
            job->OnRefereeAdded ();
          }
        }

        RefreshJobStatus (job);

        if (assigned_to_piste == FALSE)
        {
          _piste->OnSlotAssigned (this);
        }

        _piste->OnSlotUpdated (this);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Slot::RemoveJob (Job *job)
  {
    if (job)
    {
      while (_referee_list)
      {
        EnlistedReferee *referee = (EnlistedReferee *) _referee_list->data;

        RemoveReferee (referee);
      }

      _job_list = g_list_remove (_job_list,
                                 job);

      RefreshJobStatus (job);

      job->RemoveObjectListener (this);
      job->SetSlot              (nullptr);

      _piste->OnSlotUpdated (this);

      _duration = 0;

      if (GetJobList () == nullptr)
      {
        _piste->OnSlotRetracted (this);
      }
    }
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

        job->OnRefereeAdded ();
        RefreshJobStatus (job);

        current = g_list_next (current);
      }
    }

    _piste->OnSlotUpdated (this);
  }

  // --------------------------------------------------------------------------------
  void Slot::RemoveReferee (EnlistedReferee *referee)
  {
    _referee_list = g_list_remove (_referee_list,
                                   referee);

    for (GList *current = _job_list; current; current = g_list_next (current))
    {
      Job *job = (Job *) current->data;

      job->OnRefereeRemoved ();
      referee->OnRemovedFromSlot (this);
      RefreshJobStatus (job);
    }

    _piste->OnSlotUpdated (this);
  }

  // --------------------------------------------------------------------------------
  void Slot::OnObjectDeleted (Object *object)
  {
    RemoveJob ((Job *) object);
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
    if (a == nullptr)
    {
      return b;
    }
    if (b == nullptr)
    {
      return a;
    }

    if (g_date_time_compare (a, b) >= 0)
    {
      return a;
    }

    return b;
  }

  // --------------------------------------------------------------------------------
  gboolean Slot::TimeIsInside (GDateTime *time)
  {
    if (g_date_time_compare (_start, time) <= 0)
    {
      GDateTime *end_time = g_date_time_add (_start,
                                             GetDuration  ());

      if (g_date_time_compare (time,
                               end_time) < 0)
      {
        g_date_time_unref (end_time);
        return TRUE;
      }
      g_date_time_unref (end_time);
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  Slot *Slot::GetSlotAt (GDateTime *start_time,
                         GList     *slots)
  {
    GList *current = slots;

    while (current)
    {
      Slot *slot = (Slot *) current->data;

      if (slot->TimeIsInside (start_time))
      {
        return slot;
      }

      current = g_list_next (current);
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  Slot *Slot::GetFreeSlot (Piste     *piste,
                           GList     *booked_slots,
                           GDateTime *from,
                           GTimeSpan  duration)
  {
    Slot  *slot  = nullptr;
    GList *slots = GetFreeSlots (piste,
                                 booked_slots,
                                 from,
                                 duration);
    GList *current = slots;

    while (current)
    {
      slot = (Slot *) current->data;

      if (   (slot->_duration >= duration)
          && (g_date_time_compare (from, slot->_start) == 0))
      {
        slot->Retain ();
        break;
      }

      slot = nullptr;
      current = g_list_next (current);
    }

    FreeFullGList (Slot, slots);

    return slot;
  }

  // --------------------------------------------------------------------------------
  GList *Slot::GetFreeSlots (Piste     *piste,
                             GList     *booked_slots,
                             GDateTime *from,
                             GTimeSpan  duration)
  {
    GList *slots = nullptr;

    // Before the booked slots
    if (booked_slots == nullptr)
    {
      Slot *free_slot = new Slot (piste,
                                  from,
                                  nullptr,
                                  duration);
      slots = g_list_append (slots,
                             free_slot);
    }
    else
    {
      Slot      *first_slot = (Slot *) booked_slots->data;
      GDateTime *end_time   = g_date_time_add (from, duration);

      if (g_date_time_compare (first_slot->_start, end_time) >= 0)
      {
        Slot *free_slot = new Slot (piste,
                                    from,
                                    first_slot->_start,
                                    duration);
        slots = g_list_append (slots,
                               free_slot);
      }
      g_date_time_unref (end_time);
    }

    {
      GList *current = booked_slots;

      while (current)
      {
        GList *next         = g_list_next (current);
        Slot  *current_slot = (Slot *) current->data;
        Slot  *free_slot    = nullptr;

        if (next == nullptr)
        {
          // After the booked slots
          free_slot = new Slot (piste,
                                GetLatestDate (from, current_slot->_end),
                                nullptr,
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

          if (   (g_date_time_compare (max_start_time, current_slot->_end) >= 0)
              && (g_date_time_compare (max_start_time, from) >= 0))
          {
            free_slot = new Slot (piste,
                                  GetLatestDate (from, current_slot->_end),
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
    }

    return slots;
  }

  // --------------------------------------------------------------------------------
  Piste *Slot::GetPiste ()
  {
    return _piste;
  }

  // --------------------------------------------------------------------------------
  void Slot::RefreshJobStatus (Job *job)
  {
    job->RefreshStatus ();

    {
      Batch *batch = job->GetBatch ();

      batch->OnNewJobStatus (job);
    }
  }

  // --------------------------------------------------------------------------------
  void Slot::Dump ()
  {
    {
      gchar *start = g_date_time_format (_start, "%R");

      printf ("%s .. ", start);
      g_free (start);

      if (_end)
      {
        gchar *end = g_date_time_format (_end, "%R");

        printf ("%s" ESC, end);
        g_free (end);
      }
      else
      {
        printf ("~");
      }
    }

    if (_piste)
    {
      printf (" Piste %d", _piste->GetId ());
    }

    if (_referee_list)
    {
      gchar *name = ((EnlistedReferee *) _referee_list->data)->GetName ();

      printf (" %s", name);
      g_free (name);
    }

    printf ("\n");
  }
}
