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

#include <gtk/gtk.h>

#include "object.hpp"

class DndConfig : public Object
{
  public:
    struct Listener
    {
      virtual void OnDragAndDropEnd () = 0;
    };

  public:
    DndConfig ();

    guint32 AddTarget (const gchar *target,
                       guint        flags);

    void SetOnAWidgetSrc (GtkWidget       *widget,
                          GdkModifierType  start_button_mask,
                          GdkDragAction    actions);

    void SetOnAWidgetDest (GtkWidget     *widget,
                           GdkDragAction  actions);

    void SetOnAWidgetSrc (GtkTreeView     *widget,
                          GdkModifierType  start_button_mask,
                          GdkDragAction    actions);

    void SetOnAWidgetDest (GtkTreeView   *widget,
                           GdkDragAction  actions);

    guint GetTargetTableSize ();

    void SetFloatingObject (Object *object);

    Object *GetFloatingObject ();

    void SetContext (GdkDragContext *context);

    void SetPeerListener (Listener *peer);

    void DragEnd ();

  private:
    GtkTargetList *_target_list;
    Object        *_floating_object;
    Listener      *_peer_listener;

    ~DndConfig () override;
};
