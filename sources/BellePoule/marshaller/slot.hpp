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

#pragma once

#include "util/object.hpp"

class FieTime;

namespace Marshaller
{
  class Job;
  class EnlistedReferee;
  class Piste;

  class Slot :
    public Object,
    public Object::Listener
  {
    public:
      Slot (Piste     *piste,
            GDateTime *start_time,
            GDateTime *end_time,
            GTimeSpan  duration);

      void AddJob (Job *job);

      void RemoveJob (Job *job);

      GList *GetJobList ();

      void AddReferee (EnlistedReferee *referee);

      void RemoveReferee (EnlistedReferee *referee);

      GList *GetRefereeList ();

      Piste *GetPiste ();

      void Cancel ();

      GDateTime *GetStartTime ();

      GDateTime *GetEndTime ();

      GTimeSpan GetDuration ();

      gboolean CanWrap (Slot *what);

      void SetDuration (GTimeSpan duration,
                        gboolean  fix_overlaps = FALSE);

      void TailWith (Slot      *with,
                     GTimeSpan  duration);

      gboolean IsOver ();

      gboolean TimeIsInside (GDateTime *time);

      void FixOverlaps (gboolean dry_run);

      static void Dump (Slot *what);

      static gint CompareAvailbility (Slot *a,
                                      Slot *b);

      static Slot *GetFreeSlot (Piste     *piste,
                                GList     *busy_slots,
                                GDateTime *from,
                                GTimeSpan  duration);

      static GList *GetFreeSlots (Piste     *piste,
                                  GList     *busy_slots,
                                  GDateTime *from,
                                  GTimeSpan  duration);

      static Slot *GetSlotAt (GDateTime *time,
                              GList     *slots);

    private:
      Piste     *_piste;
      GList     *_job_list;
      GList     *_referee_list;
      FieTime   *_fie_time;
      GDateTime *_start;
      GDateTime *_end;
      GTimeSpan  _duration;

      ~Slot () override;

      void OnObjectDeleted (Object *object) override;

      void SetStartTime (GDateTime *time);

      void RefreshJobStatus (Job *job);

      static GDateTime *GetLatestDate (GDateTime *a,
                                       GDateTime *b);
  };
}
