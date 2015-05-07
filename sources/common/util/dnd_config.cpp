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
  _count           = 0;
  _target_table    = NULL;
  _floating_object = NULL;
  _target_list     = gtk_target_list_new (NULL, 0);
}

// --------------------------------------------------------------------------------
DndConfig::~DndConfig ()
{
  gtk_target_table_free (_target_table,
                         _count);

  gtk_target_list_unref (_target_list);
}

// --------------------------------------------------------------------------------
guint32 DndConfig::CreateTarget (const gchar *target,
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
void DndConfig::CreateTargetTable ()
{
  _target_table = gtk_target_table_new_from_list (_target_list,
                                                  &_count);
}

// --------------------------------------------------------------------------------
GtkTargetEntry *DndConfig::GetTargetTable ()
{
  if (_target_table == NULL)
  {
    CreateTargetTable ();
  }

  return _target_table;
}

// --------------------------------------------------------------------------------
guint DndConfig::GetTargetTableSize ()
{
  if (_target_table == NULL)
  {
    CreateTargetTable ();
  }

  return _count;
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
