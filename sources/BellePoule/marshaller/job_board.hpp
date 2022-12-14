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
#include "util/module.hpp"
#include "job_details.hpp"
#include "job.hpp"

namespace Marshaller
{
  class Message;
  class Timeline;
  class Competition;

  class JobBoard : public Module,
                          Job::Listener,
                          JobDetails::Listener,
                          Object::Listener
  {
    public:
      struct Listener
      {
        virtual void OnJobBoardUpdated (Competition *competition) = 0;
        virtual void OnJobBoardFocus   (guint focus) = 0;
        virtual void OnJobMove         (Job *job) = 0;
      };

    public:
      JobBoard ();

      void AddJob (Job *job);

      void AddJobs (GList *jobs);

      void Display (Job *job = nullptr);

      void Clean ();

      void OnPreviousClicked ();

      void OnNextClicked ();

      void OnTakeOffClicked ();

      void OnMoveClicked ();

      void OnCloseClicked ();

      static void ForeRedraw ();

      static void SetTimeLine (Timeline *timeline,
                               Listener *listener);

    private:
      static Timeline *_timeline;
      static Listener *_listener;
      static GList    *_boards;

      GtkWidget  *_dialog;
      GList      *_job_list;
      GList      *_current_job;
      JobDetails *_referee_details;
      JobDetails *_fencer_details;
      GtkWidget  *_take_off_button;
      GtkWidget  *_move_button;

      ~JobBoard () override;

      void OnObjectDeleted (Object *object) override;

      void SetHeader ();

      static void ForgetJob (Job      *job,
                             JobBoard *job_board);

      static void SetProperty (GQuark    key_id,
                               gchar    *data,
                               JobBoard *job_board);

      void ForgetCurrentJob ();

      void DisplayCurrent ();

      void SetSensitivity ();

      void OnPlayerAdded (Player *player) override;

      void OnPlayerRemoved (Player *player) override;

      JobDetails *GetPairedOf (JobDetails *of) override;

      void OnJobUpdated (Job *job) override;
  };
}
