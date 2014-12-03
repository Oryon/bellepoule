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

#include "glade.hpp"

guint Glade::_stamp = 1;

// --------------------------------------------------------------------------------
Glade::Glade (const gchar *file_name,
              Object      *owner)
: Object ("Glade")
{
  if (file_name)
  {
    GError *error = NULL;
    gchar  *path = g_build_filename (_share_dir, "resources", "glade", file_name, NULL);

    _glade_xml = gtk_builder_new ();

    gtk_builder_add_from_file (_glade_xml,
                               path,
                               &error);
    if (error != NULL)
    {
      g_print ("<<%s>> %s\n", path, error->message);
    }
    g_free (path);

    if (error != NULL)
    {
      gchar *spare_file_name = g_build_filename (_share_dir, "..", "..", "resources", "glade", file_name, NULL);

      g_clear_error (&error);
      gtk_builder_add_from_file (_glade_xml,
                                 spare_file_name,
                                 &error);
      g_free (spare_file_name);

      if (error != NULL)
      {
        g_print ("<<%s>> %s\n", spare_file_name, error->message);
        g_error_free (error);

        gtk_main_quit ();
      }
    }

    gtk_builder_connect_signals (_glade_xml,
                                 owner);
#ifdef DEBUG
    StampAllWidgets ();
#endif
  }
  else
  {
    _glade_xml = NULL;
  }
}

// --------------------------------------------------------------------------------
Glade::~Glade ()
{
  if (_glade_xml)
  {
    {
      GSList *objects = gtk_builder_get_objects (_glade_xml);
      GSList *current = objects;

      while (current)
      {
        GObject *object = (GObject *) current->data;

        if (GTK_IS_WIDGET (object))
        {
          GtkWidget *widget = GTK_WIDGET (object);

          if (gtk_widget_is_toplevel (widget))
          {
            gtk_widget_destroy (widget);
          }
        }

        current = g_slist_next (current);
      }

      g_slist_free (objects);
    }

    g_object_unref (G_OBJECT (_glade_xml));
  }
}

// --------------------------------------------------------------------------------
GSList *Glade::GetObjectList ()
{
  return gtk_builder_get_objects (_glade_xml);
}

// --------------------------------------------------------------------------------
void Glade::StampAllWidgets ()
{
  GSList *objects = gtk_builder_get_objects (_glade_xml);
  GSList *current = objects;

  while (current)
  {
    GObject *object = (GObject *) current->data;

    if (GTK_IS_WIDGET (object))
    {
      gchar *stamp_string;

      if (GTK_IS_BUILDABLE (object))
      {
        stamp_string = g_strdup_printf ("%d::%s::%s",
                                        _stamp,
                                        G_OBJECT_TYPE_NAME (object),
                                        gtk_buildable_get_name (GTK_BUILDABLE (object)));
      }
      else
      {
        stamp_string = g_strdup_printf ("%d::%s",
                                        _stamp,
                                        G_OBJECT_TYPE_NAME (object));
      }

      gtk_widget_set_name (GTK_WIDGET (object),
                           stamp_string);
      g_free (stamp_string);
    }

    current = g_slist_next (current);
  }

  g_slist_free (objects);

  _stamp++;
}

// --------------------------------------------------------------------------------
void Glade::DetachFromParent (GtkWidget *widget)
{
  if (widget)
  {
    GtkWidget *parent = gtk_widget_get_parent (widget);

    if (parent)
    {
      gtk_container_remove (GTK_CONTAINER (parent),
                            widget);
    }
  }
}

// --------------------------------------------------------------------------------
GtkWidget *Glade::GetWidget (const gchar *name)
{
  GtkWidget *widget = (GtkWidget *) (gtk_builder_get_object (_glade_xml,
                                                             name));

#if 0
  if (widget)
  {
    gtk_widget_set_name (widget,
                         name);
  }
#endif

  return widget;
}

// --------------------------------------------------------------------------------
GObject *Glade::GetGObject (const gchar *name)
{
  return gtk_builder_get_object (_glade_xml,
                                 name);
}

// --------------------------------------------------------------------------------
GtkWidget *Glade::GetRootWidget ()
{
  return GetWidget ("root");
}
