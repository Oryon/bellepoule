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

#include "slot.hpp"
#include "job.hpp"
#include "job_details.hpp"

#include "job_board.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  JobBoard::JobBoard ()
    : Module ("job_board.glade")
  {
    _dialog = _glade->GetWidget ("dialog");
  }

  // --------------------------------------------------------------------------------
  JobBoard::~JobBoard ()
  {
  }

  // --------------------------------------------------------------------------------
  void JobBoard::Display (GList *job_list)
  {
    Job *job = (Job *) job_list->data;

    Display (job);
  }

  // --------------------------------------------------------------------------------
  void JobBoard::Display (Job *job)
  {
    JobDetails *referee_details = NULL;
    JobDetails *fencer_details  = NULL;
    Slot       *slot            = job->GetSlot ();

    if (slot)
    {
      referee_details = new JobDetails (slot->GetRefereeList ());

      Plug (referee_details,
            _glade->GetWidget ("referee_detail_hook"));
    }

    {
      fencer_details  = new JobDetails (job->GetFencerList ());

      Plug (fencer_details,
            _glade->GetWidget ("fencer_detail_hook"));
    }

    gtk_dialog_run (GTK_DIALOG (_dialog));
    gtk_widget_hide (_dialog);

    Object::TryToRelease (referee_details);
    Object::TryToRelease (fencer_details);
  }
}
