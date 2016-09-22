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

#ifndef batch_hpp
#define batch_hpp

#include <libxml/xpath.h>

#include "network/message.hpp"
#include "util/module.hpp"

class Player;

namespace Marshaller
{
  class Job;

  class Batch : public Module
  {
    public:
      struct Listener
      {
        virtual void OnBatchAssignmentRequest   (Batch *batch) = 0;
        virtual void OnBatchAssignmentCancel    (Batch *batch) = 0;
        virtual void OnJobDetailsDisplayRequest (Job   *job)   = 0;
      };

    public:
      Batch (const gchar *id,
             Listener    *listener);

      void AttachTo (GtkNotebook *to);

      void Load (Net::Message *message);

      void SetVisibility (Job      *job,
                          gboolean  visibility);

      void RemoveJob (Net::Message *message);

      guint GetId ();

      GList *GetScheduledJobs ();

      GList *GetPendingJobs ();

      const gchar *GetName ();

      void SetProperties (Net::Message *message);

      GList *GetCurrentSelection ();

      void OnAssign ();

      void OnCancelAssign ();

      GdkColor *GetColor ();

      const gchar *GetWeaponCode ();

      void ManageFencer (Net::Message *message);

      void DeleteFencer (Net::Message *message);

      void on_batch_treeview_row_activated (GtkTreePath *path);

    private:
      guint32       _id;
      GtkListStore *_job_store;
      guint32       _dnd_key;
      GdkColor     *_gdk_color;
      gchar        *_name;
      Listener     *_listener;
      GList        *_scheduled_list;
      GList        *_pending_list;
      GList        *_fencer_list;
      gchar        *_weapon;

      virtual ~Batch ();

      void LoadJob (xmlNode *xml_node,
                    guint    uuid,
                    Job     *job = NULL);

      Job *GetJob (guint uuid);

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

#endif
