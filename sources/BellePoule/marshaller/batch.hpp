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
    Batch (const gchar *id);

    void AttachTo (GtkNotebook *to);

    void LoadJob (Net::Message *message);

    void SetVisibility (Job      *job,
                        gboolean  visibility);

    guint GetId ();

    const gchar *GetName ();

    void SetProperties (Net::Message *message);

    GSList *GetCurrentSelection ();

    GdkColor *GetColor ();

  private:
    guint32       _id;
    GtkListStore *_job_store;
    guint32       _dnd_key;
    GdkColor     *_gdk_color;
    gchar        *_name;

    virtual ~Batch ();

    void LoadJob (xmlNode *xml_node);

    void SetProperty (Net::Message *message,
                      const gchar  *property);

    void OnDragDataGet (GtkWidget        *widget,
                        GdkDragContext   *drag_context,
                        GtkSelectionData *data,
                        guint             key,
                        guint             time);
};

#endif
