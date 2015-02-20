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
Batch::Batch (const gchar *id)
  : Module ("batch.glade")
{
  _id = g_strdup (id);

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
  g_free (_id);
}

// --------------------------------------------------------------------------------
const gchar *Batch::GetId ()
{
  return _id;
}

// --------------------------------------------------------------------------------
void Batch::SetProperty (GKeyFile    *key_file,
                         const gchar *property)
{
  gchar    *property_widget = g_strdup_printf ("contest_%s_label", property);
  GtkLabel *label           = GTK_LABEL (_glade->GetGObject (property_widget));
  gchar    *value;

  value = g_key_file_get_string (key_file,
                                 "Contest",
                                 property,
                                 NULL);
  gtk_label_set_text (label,
                      gettext (value));

  g_free (property_widget);
  g_free (value);
}

// --------------------------------------------------------------------------------
void Batch::SetProperties (GKeyFile *key_file)
{
  SetProperty (key_file, "gender");
  SetProperty (key_file, "weapon");
  SetProperty (key_file, "category");

  {
    GdkColor  *gdk_color;
    GtkWidget *tab   = _glade->GetWidget ("notebook_title");
    gchar     *color = g_key_file_get_string (key_file,
                                              "Contest",
                                              "color",
                                              NULL);
    gdk_color_parse (color,
                     gdk_color);

    gtk_widget_modify_bg (tab,
                          GTK_STATE_NORMAL,
                          gdk_color);
    gtk_widget_modify_bg (tab,
                          GTK_STATE_ACTIVE,
                          gdk_color);

    gdk_color_free (gdk_color);
    g_free (color);
  }
}

// --------------------------------------------------------------------------------
void Batch::AttachTo (GtkNotebook *to)
{
  gtk_notebook_append_page (to,
                            GetRootWidget (),
                            _glade->GetWidget ("notebook_title"));
}
