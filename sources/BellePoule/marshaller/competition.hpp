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

class Player;
class FieTime;

namespace Marshaller
{
  class Job;
  class JobBoard;

  class Competition : public Module
  {
    public:
      struct Listener
      {
        virtual gboolean OnCompetitionAssignmentRequest (Competition *competition) = 0;
        virtual void     OnCompetitionAssignmentCancel  (Competition *competition) = 0;
      };

    public:
      Competition (guint     id,
                   Listener *listener);

      void AttachTo (GtkNotebook *to);

      Job *Load (Net::Message  *message,
                 guint         *piste_id,
                 guint         *referee_id,
                 FieTime      **start_time);

      void SetJobStatus (Job      *job,
                         gboolean  has_slot,
                         gboolean  has_referee);

      void RemoveJob (Net::Message *message);

      guint GetId ();

      GList *GetScheduledJobs ();

      GList *GetPendingJobs ();

      const gchar *GetName ();

      void SetProperties (Net::Message *message);

      GList *GetCurrentSelection ();

      void OnAssign ();

      void OnCancelAssign ();

      void OnValidateAssign ();

      GdkColor *GetColor ();

      GData *GetProperties ();

      const gchar *GetWeaponCode ();

      void ManageFencer (Net::Message *message);

      void DeleteFencer (Net::Message *message);

      void on_competition_treeview_row_activated (GtkTreePath *path);

    private:
      guint         _id;
      GtkListStore *_job_store;
      guint32       _dnd_key;
      GdkColor     *_gdk_color;
      gchar        *_name;
      Listener     *_listener;
      GList        *_scheduled_list;
      GList        *_pending_list;
      GList        *_fencer_list;
      gchar        *_weapon;
      JobBoard     *_job_board;
      GData        *_properties;
      GtkWidget    *_assign_button;
      GtkWidget    *_cancel_button;
      GtkWidget    *_lock_button;

      virtual ~Competition ();

      Job *GetJob (guint netid);

      void RefreshControlPanel ();

      Player *GetFencer (guint ref);

      void SetProperty (Net::Message *message,
                        const gchar  *property);

      void OnDragDataGet (GtkWidget        *widget,
                          GdkDragContext   *drag_context,
                          GtkSelectionData *data,
                          guint             key,
                          guint             time);
  };
}
