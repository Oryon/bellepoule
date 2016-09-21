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

#include "job.hpp"

#include "job_details.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  JobDetails::JobDetails (GList *player_list)
    : PlayersList ("details.glade",
                   SORTABLE)
  {
    {
      GSList *attr_list;
      Filter *filter;

      AttributeDesc::CreateExcludingList (&attr_list,
#ifndef DEBUG
                                          "ref",
#endif
                                          "IP",
                                          "HS",
                                          "attending",
                                          "availability",
                                          "exported",
                                          "global_status",
                                          "indice",
                                          "level",
                                          "workload_rate",
                                          "pool_nr",
                                          "stage_start_rank",
                                          "promoted",
                                          "rank",
                                          "status",
                                          "team",
                                          "victories_count",
                                          "bouts_count",
                                          "victories_ratio",
                                          NULL);
      filter = new Filter (attr_list,
                           this);

      filter->ShowAttribute ("name");
      filter->ShowAttribute ("club");
      filter->ShowAttribute ("league");
      filter->ShowAttribute ("country");

      SetFilter (filter);
      filter->Release ();
    }

    {
      GList *current = player_list;

      while (current)
      {
        Player *player= (Player *) current->data;

        Add (player);

        current = g_list_next (current);
      }
    }
  }

  // --------------------------------------------------------------------------------
  JobDetails::~JobDetails ()
  {
  }

  // --------------------------------------------------------------------------------
  void JobDetails::OnPlugged ()
  {
    OnAttrListUpdated ();
  }
}
