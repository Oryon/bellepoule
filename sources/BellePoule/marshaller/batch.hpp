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

  class Batch : public Module
  {
    public:
      struct Listener
      {
        virtual gboolean OnBatchAssignmentRequest (Batch *batch) = 0;
        virtual void     OnBatchAssignmentCancel  (Batch *batch) = 0;
      };

      enum Status
      {
        UNCOMPLETED,
        CONCEALED,
        DISCLOSED
      };

    public:
      Batch (Net::Message *message,
             Competition  *competition,
             Listener     *listener);

      Competition *GetCompetition ();

      Job *Load (Net::Message  *message,
                 guint         *piste_id,
                 guint         *referee_id,
                 FieTime      **start_time);

      void CloseLoading ();

      void OnNewJobStatus (Job *job);

      void RemoveJob (Net::Message *message);

      guint GetId ();

      GList *GetScheduledJobs ();

      GList *GetPendingJobs ();

      Job *GetJob (guint netid);

      const gchar *GetName ();

      GList *GetCurrentSelection ();

      void OnAssign ();

      void OnCancelAssign ();

      void OnValidateAssign ();

      gboolean IsModifiable ();

      void on_competition_treeview_row_activated (GtkTreePath *path);

    private:
      guint         _id;
      Competition  *_competition;
      GtkListStore *_job_store;
      gchar        *_name;
      Listener     *_listener;
      GList        *_scheduled_list;
      GList        *_pending_list;
      JobBoard     *_job_board;
      GtkWidget    *_assign_button;
      GtkWidget    *_cancel_button;
      gboolean      _loading;

      virtual ~Batch ();

      void RecallList (GList *list);

      void RefreshControlPanel ();

      void OnDragDataGet (GtkWidget        *widget,
                          GdkDragContext   *drag_context,
                          GtkSelectionData *data,
                          guint             key,
                          guint             time);
  };
}
