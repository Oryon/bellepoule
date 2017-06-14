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
#include "job_details.hpp"

namespace Marshaller
{
  class Message;
  class Job;
  class Timeline;
  class Competition;

  class JobBoard : public Module,
                          JobDetails::Listener,
                          Object::Listener
  {
    public:
      struct Listener
      {
        virtual void OnJobBoardUpdated (Competition *competition) = 0;
        virtual void OnJobBoardFocus   (guint focus) = 0;
      };

    public:
      JobBoard ();

      void AddJob (Job *job);

      void AddJobs (GList *jobs);

      void Display (Job *job = NULL);

      void Clean ();

      void OnPreviousClicked ();

      void OnNextClicked ();

      static void SetTimeLine (Timeline *timeline,
                               Listener *listener);

    private:
      static Timeline *_timeline;
      static Listener *_listener;

      GtkWidget  *_dialog;
      GList      *_job_list;
      GList      *_current_job;
      JobDetails *_referee_details;
      JobDetails *_fencer_details;
      GtkWidget  *_take_off_button;

      ~JobBoard ();

      void OnObjectDeleted (Object *object);

      void SetHeader ();

      static void ForgetJob (Job      *job,
                             JobBoard *job_board);

      static void SetProperty (GQuark    key_id,
                               gchar    *data,
                               JobBoard *job_board);

      void DisplayCurrent ();

      void OnPlayerRemoved (Player *player);
  };
}
