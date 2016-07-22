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

#ifndef slot_hpp
#define slot_hpp

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
            GTimeSpan  duration);

      void AddJob (Job *job);

      void RemoveJob (Job *job);

      GList *GetJobList ();

      void AddReferee (EnlistedReferee *referee);

      GList *GetRefereeList ();

      void Cancel ();

      GDateTime *GetStartTime ();

      GTimeSpan GetDuration ();

      GTimeSpan GetInterval (Slot *with);

      gboolean Overlaps (Slot *what);

      static void Dump (Slot *what);

      static gint CompareAvailbility (Slot *a,
                                      Slot *b);

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
  };
}

#endif
