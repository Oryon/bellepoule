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

#include <libxml/xpath.h>

#include "network/message.hpp"
#include "util/module.hpp"

class FieTime;

namespace Marshaller
{
  class Competition;
  class Job;
  class JobBoard;
  class EndOfBurst;

  class Batch : public Module
  {
    public:
      struct Listener
      {
        virtual gboolean OnBatchAssignmentRequest (Batch *batch) = 0;
        virtual void     OnBatchAssignmentCancel  (Batch *batch) = 0;
        virtual void     OnJobOverlapWarning      (Batch *batch) = 0;
      };

      enum class Status
      {
        INCOMPLETE,
        COMPLETE
      };

    public:
      Batch (Net::Message *message,
             Competition  *competition,
             Listener     *listener);

      Competition *GetCompetition ();

      Job *Load (Net::Message  *message,
                 guint         *piste_id,
                 GList        **referees,
                 FieTime      **start_time);

      void Mute ();

      void UnMute ();

      void OnNewJobStatus (Job *job);

      void OnNewAffinitiesRule (Job *job);

      void RemoveJob (Net::Message *message);

      guint GetId ();

      GList *GetScheduledJobs ();

      GList *RetreivePendingJobs ();

      Job *GetJob (guint netid);

      const gchar *GetName ();

      GList *RetreivePendingSelected ();

      void OnAssign ();

      void OnExtension ();

      void OnCancelAssign ();

      void Recall () override;

      void Spread () override;

      gboolean IsModifiable ();

      void RaiseOverlapWarning ();

      void FixOverlapWarnings ();

      Status GetStatus ();

      void on_competition_treeview_row_activated (GtkTreePath *path);

    private:
      guint         _id;
      guint         _stage;
      gchar        *_batch_ref;
      Competition  *_competition;
      GtkListStore *_job_store;
      gchar        *_name;
      Listener     *_listener;
      GList        *_scheduled_list;
      GList        *_pending_list;
      JobBoard     *_job_board;
      GtkWidget    *_assign_button;
      GtkWidget    *_cancel_button;
      guint         _expected_jobs;
      EndOfBurst   *_eob;
      Status        _status;

      ~Batch () override;

      void RefreshControlPanel ();

      void OnDragDataGet (GtkWidget        *widget,
                          GdkDragContext   *drag_context,
                          GtkSelectionData *data,
                          guint             key,
                          guint             time) override;
  };
}
