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

#include <glib/gi18n.h>

#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "util/object.hpp"

// --------------------------------------------------------------------------------
#ifdef DEBUG
static void LogHandler (const gchar    *log_domain,
                        GLogLevelFlags  log_level,
                        const gchar    *message,
                        gpointer        user_data)
{
  switch (log_level)
  {
    case G_LOG_FLAG_FATAL:
    case G_LOG_LEVEL_ERROR:
    case G_LOG_LEVEL_CRITICAL:
    case G_LOG_LEVEL_WARNING:
    case G_LOG_FLAG_RECURSION:
    {
      g_print (RED "[%s]" ESC " %s\n", log_domain, message);
    }
    break;

    case G_LOG_LEVEL_MESSAGE:
    case G_LOG_LEVEL_INFO:
    case G_LOG_LEVEL_DEBUG:
    case G_LOG_LEVEL_MASK:
    default:
    {
      g_print (BLUE "[%s]" ESC " %s\n", log_domain, message);
    }
    break;
  }
}
#endif

// --------------------------------------------------------------------------------
int main (int argc, char **argv)
{
  gchar      *install_dirname;
  GtkBuilder *_glade_xml;

  // g_mem_set_vtable (glib_mem_profiler_table);

  // Init
  {
    {
      gchar *binary_dir;

      {
        gchar *program = g_find_program_in_path (argv[0]);

        binary_dir = g_path_get_dirname (program);
        g_free (program);
      }

#ifdef DEBUG
      g_log_set_default_handler (LogHandler,
                                 NULL);
      install_dirname = g_build_filename (binary_dir, "..", NULL);
#else
      {
        gchar *basename = g_path_get_basename (argv[0]);

        if (strstr (basename, ".exe"))
        {
          g_free (basename);
          basename = g_strdup ("bellepoule");
        }

        install_dirname = g_build_filename (binary_dir, "..", "share", basename, NULL);
        g_free (basename);
      }
#endif
      g_free (binary_dir);

      Object::SetProgramPath (install_dirname);
    }

    {
      gtk_init (&argc, &argv);

      g_type_class_unref (g_type_class_ref (GTK_TYPE_IMAGE_MENU_ITEM));
      g_object_set (gtk_settings_get_default (), "gtk-menu-images", TRUE, NULL);
    }

    //Object::Track ("Player");
  }

  {
    GError *error = NULL;
    gchar  *path = g_build_filename (install_dirname, "resources", "LivePoule.glade", NULL);

    _glade_xml = gtk_builder_new ();

    gtk_builder_add_from_file (_glade_xml,
                               path,
                               &error);
    if (error != NULL)
    {
      g_print ("<<%s>> %s\n", path, error->message);
      g_error_free (error);

      gtk_main_quit ();
    }
    g_free (path);

    gtk_builder_connect_signals (_glade_xml,
                                 NULL);
  }

  {
    GtkWidget *widget = (GtkWidget *) (gtk_builder_get_object (_glade_xml,
                                                               "root"));

    {
      GdkRGBA green = {0.137, 0.568, 0.137, 1.0};

      gtk_widget_override_background_color ((GtkWidget *) gtk_builder_get_object (_glade_xml, "green_light_viewport"),
                                            GTK_STATE_FLAG_NORMAL,
                                            &green);
    }
    {
      GdkRGBA red = {0.568, 0.137, 0.137, 1.0};

      gtk_widget_override_background_color ((GtkWidget *) gtk_builder_get_object (_glade_xml, "red_light_viewport"),
                                            GTK_STATE_FLAG_NORMAL,
                                            &red);
    }
    {
      GdkRGBA purple = {(double) 0xAD / 255.0, (double) 0xA7 / 255.0, (double) 0xC8 / 255.0, 1.0};

      gtk_widget_override_background_color ((GtkWidget *) gtk_builder_get_object (_glade_xml, "title_viewport"),
                                            GTK_STATE_FLAG_NORMAL,
                                            &purple);
    }

    gtk_widget_show_all (widget);
  }

  gtk_main ();

  g_free (install_dirname);
  g_object_unref (G_OBJECT (_glade_xml));

  return 0;
}
