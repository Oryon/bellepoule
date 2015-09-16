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

#ifndef table_set_border_hpp
#define table_set_border_hpp

#include <gtk/gtk.h>

#include "util/object.hpp"

#include "table.hpp"

namespace Table
{
  class TableSetBorder : public Object
  {
    public:
      TableSetBorder (Object    *owner,
                      GCallback  callback,
                      GtkWidget *container,
                      GtkWidget *widget);

      void SetTableIcon (guint        table,
                         const gchar *icon);

      void MuteCallbacks ();

      void AddTable (Table *table);

      void UnMuteCallbacks ();

      void SelectTable (gint table);

      Table *GetSelectedTable ();

      void Plug ();

      void UnPlug ();

    private:
      Object       *_owner;
      GCallback     _callback;
      GtkListStore *_liststore;
      GtkContainer *_container;
      GtkWidget    *_widget;

      virtual ~TableSetBorder ();
  };
}

#endif
