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

#include "util/player.hpp"
#include "actors/referee.hpp"

namespace Marshaller
{
  class JobBoard;
  class Slot;

  class EnlistedReferee : public Referee
  {
    public:
      EnlistedReferee ();

      void AddSlot (Slot *slot);

      gboolean IsAvailableFor (Slot      *slot,
                               GTimeSpan  duration);

      Slot *GetAvailableSlotFor (Slot      *slot,
                                 GTimeSpan  duration);

      gint GetWorkload ();

      void SetAllRefereWorkload (gint all_referee_workload);

      static void OnRemovedFromSlot (EnlistedReferee *referee,
                                     Slot            *slot);

      static gint CompareWorkload (EnlistedReferee *a,
                                   EnlistedReferee *b);

      void DisplayJobs ();

    private:
      GList       *_slots;
      AttributeId *_workload_rate_attr_id;
      gint         _work_load;
      JobBoard    *_job_board;

      virtual ~EnlistedReferee ();
  };
}
