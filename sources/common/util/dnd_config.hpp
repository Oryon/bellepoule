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

#ifndef dnd_config_hpp
#define dnd_config_hpp

#include <gtk/gtk.h>

#include "object.hpp"

class DndConfig : public virtual Object
{
  public:
    DndConfig ();

    guint32 CreateTarget (const gchar *target,
                          guint        flags);

    GtkTargetEntry *GetTargetTable ();

    guint GetTargetTableSize ();

    void SetFloatingObject (Object *object);

    Object *GetFloatingObject ();


  private:
    GtkTargetList  *_target_list;
    GtkTargetEntry *_target_table;
    gint            _count;
    Object         *_floating_object;

    ~DndConfig ();

    void CreateTargetTable ();
};

#endif
