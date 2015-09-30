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

class Job;

class Batch : public Module
{
  public:
    class Listener
    {
      public:
        virtual void OnBatchAssignmentRequest (Batch *batch) = 0;
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

    GList *RetreiveJobList ();

    const gchar *GetName ();

    void SetProperties (Net::Message *message);

    GSList *GetCurrentSelection ();

    void OnAssign ();

    void OnCancelAssign ();

  private:
    guint32       _id;
    GtkListStore *_job_store;
    guint32       _dnd_key;
    GdkColor     *_gdk_color;
    gchar        *_name;
    Listener     *_listener;

    virtual ~Batch ();

    void LoadJob (xmlNode *xml_node,
                  guint    uuid);

    void SetProperty (Net::Message *message,
                      const gchar  *property);

    void OnDragDataGet (GtkWidget        *widget,
                        GdkDragContext   *drag_context,
                        GtkSelectionData *data,
                        guint             key,
                        guint             time);
};

#endif
