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

#include <gtk/gtk.h>
#include <glade/glade.h>

#include "glade.hpp"

// --------------------------------------------------------------------------------
Glade_c::Glade_c (gchar *file_name)
{
  if (file_name)
  {
    _glade_xml = glade_xml_new (file_name,
                                NULL,
                                NULL);

    if (_glade_xml == NULL)
    {
      gchar *spare_file_name = g_strdup_printf ("../../%s", file_name);

      _glade_xml = glade_xml_new (spare_file_name,
                                  NULL,
                                  NULL);
      g_free (spare_file_name);

      if (_glade_xml == NULL)
      {
        gtk_main_quit ();
      }
    }

    glade_xml_signal_autoconnect (_glade_xml);
  }
  else
  {
    _glade_xml = NULL;
  }
}

// --------------------------------------------------------------------------------
Glade_c::~Glade_c ()
{
}

// --------------------------------------------------------------------------------
void Glade_c::DetachFromParent (GtkWidget *widget)
{
  GtkWidget *parent = gtk_widget_get_parent (widget);

  if (parent)
  {
    gtk_container_forall (GTK_CONTAINER (parent),
                          (GtkCallback) g_object_ref,
                          NULL);
    gtk_container_remove (GTK_CONTAINER (parent),
                          widget);
  }
}

// --------------------------------------------------------------------------------
GtkWidget *Glade_c::GetWidget (gchar *name)
{
  return glade_xml_get_widget (_glade_xml,
                               name);
}

// --------------------------------------------------------------------------------
GtkWidget *Glade_c::GetRootWidget ()
{
  return glade_xml_get_widget (_glade_xml,
                               "root");
}

// --------------------------------------------------------------------------------
void Glade_c::Bind (gchar *widget_name,
                    void  *o)
{
  GtkWidget *w = GetWidget (widget_name);

  if (w)
  {
    g_object_set_data (G_OBJECT (w),
                       "instance",
                       o);
  }
}
