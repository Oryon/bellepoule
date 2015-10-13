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

#ifndef timeslot_hpp
#define timeslot_hpp

#include "util/object.hpp"

class Job;
class Referee;

class TimeSlot :
  public Object,
  public Object::Listener
{
  public:
    class Owner
    {
      public:
        virtual void  OnTimeSlotUpdated (TimeSlot *timeslot) = 0;
        virtual guint GetId () = 0;
    };

  public:
    TimeSlot (Owner     *owner,
              GDateTime *start_time);

    void AddJob (Job *job);

    void RemoveJob (Job *job);

    GList *GetJobList ();

    void AddReferee (Referee *referee);

    GList *GetRefereeList ();

    void Cancel ();

    GDateTime *GetStartTime ();

    GTimeSpan GetDuration ();

    static gint CompareAvailbility (TimeSlot *a,
                                    TimeSlot *b);

  private:
    Owner     *_owner;
    GList     *_job_list;
    GList     *_referee_list;
    GDateTime *_start_time;
    GTimeSpan  _duration;

    virtual ~TimeSlot ();

    void OnObjectDeleted (Object *object);
};

#endif
