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

#include "dnd_config.hpp"

// --------------------------------------------------------------------------------
DndConfig::DndConfig ()
  : Object ("DndConfig")
{
  _floating_object = NULL;
  _target_list     = gtk_target_list_new (NULL, 0);
}

// --------------------------------------------------------------------------------
DndConfig::~DndConfig ()
{
  gtk_target_list_unref (_target_list);
}

// --------------------------------------------------------------------------------
guint32 DndConfig::AddTarget (const gchar *target,
                              guint        flags)
{
  GdkAtom atom  = gdk_atom_intern (target, FALSE);
  GQuark  quark = g_quark_from_string (target);

  gtk_target_list_add (_target_list,
                       atom,
                       flags,
                       quark);

  return quark;
}

// --------------------------------------------------------------------------------
void DndConfig::SetOnAWidgetSrc (GtkWidget       *widget,
                                 GdkModifierType  start_button_mask,
                                 GdkDragAction    actions)
{
  gtk_drag_source_set (widget,
                       start_button_mask,
                       NULL, 0,
                       actions);
  gtk_drag_source_set_target_list (widget,
                                   _target_list);
}

// --------------------------------------------------------------------------------
void DndConfig::SetOnAWidgetDest (GtkWidget     *widget,
                                  GdkDragAction  actions)
{
  gtk_drag_dest_set (widget,
                     (GtkDestDefaults) 0,
                     NULL, 0,
                     actions);
  gtk_drag_dest_set_target_list (widget,
                                 _target_list);
}

// --------------------------------------------------------------------------------
void DndConfig::SetOnAWidgetSrc (GtkTreeView     *widget,
                                 GdkModifierType  start_button_mask,
                                 GdkDragAction    actions)
{
  gtk_tree_view_enable_model_drag_source (widget,
                                          start_button_mask,
                                          NULL, 0,
                                          actions);
  gtk_drag_source_set_target_list (GTK_WIDGET (widget),
                                   _target_list);
}

// --------------------------------------------------------------------------------
void DndConfig::SetOnAWidgetDest (GtkTreeView   *widget,
                                  GdkDragAction  actions)
{
  gtk_tree_view_enable_model_drag_dest (widget,
                                        NULL, 0,
                                        actions);
  gtk_drag_dest_set_target_list (GTK_WIDGET (widget),
                                 _target_list);
}

// --------------------------------------------------------------------------------
void DndConfig::SetFloatingObject (Object *object)
{
  _floating_object = object;
}

// --------------------------------------------------------------------------------
Object *DndConfig::GetFloatingObject ()
{
  return _floating_object;
}
