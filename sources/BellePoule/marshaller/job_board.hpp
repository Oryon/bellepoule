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

#include "util/module.hpp"

namespace Marshaller
{
  class Message;
  class Job;
  class JobDetails;

  class JobBoard : public Module, Object::Listener
  {
    public:
      JobBoard ();

      void AddJob (Job *job);

      void AddJobs (GList *jobs);

      void Display (Job *job = NULL);

      void Clean ();

      void OnPreviousClicked ();

      void OnNextClicked ();

    private:
      GtkWidget  *_dialog;
      GList      *_job_list;
      GList      *_current_job;
      JobDetails *_referee_details;
      JobDetails *_fencer_details;

      ~JobBoard ();

      void OnObjectDeleted (Object *object);

      void SetHeader ();

      static void ForgetJob (Job      *job,
                             JobBoard *job_board);

      static void SetProperty (GQuark    key_id,
                               gchar    *data,
                               JobBoard *job_board);

      void DisplayCurrent ();
  };
}
