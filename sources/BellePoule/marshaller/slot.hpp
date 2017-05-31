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

  class Slot :
    public Object,
    public Object::Listener
  {
    public:
      class Owner
      {
        public:
          virtual void  OnSlotUpdated (Slot *slot) = 0;
          virtual void  OnSlotLocked  (Slot *slot) = 0;
          virtual guint GetId () = 0;
      };

    public:
      Slot (Owner     *owner,
            GDateTime *start_time,
            GDateTime *end_time,
            GTimeSpan  duration);

      void AddJob (Job *job);

      void RemoveJob (Job *job);

      GList *GetJobList ();

      void AddReferee (EnlistedReferee *referee);

      GList *GetRefereeList ();

      Owner *GetOwner ();

      void Cancel ();

      GDateTime *GetStartTime ();

      GDateTime *GetEndTime ();

      GTimeSpan GetDuration ();

      gboolean FitWith (Slot *what);

      static void Dump (Slot *what);

      static gint CompareAvailbility (Slot *a,
                                      Slot *b);

      static Slot *GetFreeSlot (Owner     *owner,
                                GList     *busy_slots,
                                GDateTime *from,
                                GTimeSpan  duration);

      static GList *GetFreeSlots (Owner     *owner,
                                  GList     *busy_slots,
                                  GDateTime *from,
                                  GTimeSpan  duration);

      static GDateTime *GetAsap (GDateTime *after);

    private:
      Owner     *_owner;
      GList     *_job_list;
      GList     *_referee_list;
      FieTime   *_fie_time;
      GDateTime *_start;
      GDateTime *_end;
      GTimeSpan  _duration;

      virtual ~Slot ();

      void OnObjectDeleted (Object *object);

      void SetStartTime (GDateTime *time);

      static GDateTime *GetLatestDate (GDateTime *a,
                                       GDateTime *b);
  };
}
