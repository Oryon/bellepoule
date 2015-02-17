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

#include "batch.hpp"

typedef enum
{
  NAME_str
} ColumnId;

// --------------------------------------------------------------------------------
Batch::Batch ()
  : Module ("batch.glade")
{
  {
    _color = g_new (GdkColor, 1);
    gdk_color_parse ("#EED680", _color);
  }

  {
    GtkTreeIter iter;

    _list_store = GTK_LIST_STORE (_glade->GetGObject ("liststore"));

    for (guint i = 0; i < 5; i ++)
    {
      gtk_list_store_append (_list_store,
                             &iter);

      gtk_list_store_set (_list_store, &iter,
                          NAME_str, "Poule N°1",
                          -1);
    }
  }
}

// --------------------------------------------------------------------------------
Batch::~Batch ()
{
}

// --------------------------------------------------------------------------------
void Batch::AttachTo (GtkNotebook *to)
{
  gtk_notebook_append_page (to,
                            GetRootWidget (),
                            _glade->GetWidget ("notebook_title"));

  {
    GtkWidget *tab = _glade->GetWidget ("notebook_title");

    gtk_widget_modify_bg (tab,
                          GTK_STATE_NORMAL,
                          _color);
    gtk_widget_modify_bg (tab,
                          GTK_STATE_ACTIVE,
                          _color);
  }
}
