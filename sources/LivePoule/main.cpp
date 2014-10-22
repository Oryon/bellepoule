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

#include "util/object.hpp"
#include "util/wifi_code.hpp"
#include "screen.hpp"
#include "gpio.hpp"
#include "sg_machine.hpp"

static Screen *screen = NULL;

// --------------------------------------------------------------------------------
static void LogHandler (const gchar    *log_domain,
                        GLogLevelFlags  log_level,
                        const gchar    *message,
                        gpointer        user_data)
{
  if (log_domain == NULL)
  {
    log_domain = "LivePoule";
  }

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

// --------------------------------------------------------------------------------
int main (int argc, char **argv)
{
  // g_mem_set_vtable (glib_mem_profiler_table);

  // Init
  {
    gchar *install_dirname;

    {
      gchar *binary_dir;

      {
        gchar *program = g_find_program_in_path (argv[0]);

        binary_dir = g_path_get_dirname (program);
        g_free (program);
      }

      g_log_set_default_handler (LogHandler,
                                 NULL);
#ifdef DEBUG
      install_dirname = g_build_filename (binary_dir, "..", "..", "..", NULL);
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
    }

    gtk_init (&argc, &argv);

    //Object::Track ("Player");

    {
      Object::SetProgramPath (install_dirname);

      {
        gchar *translation_path = g_build_filename (install_dirname, "resources", "countries", "translations", NULL);

        bindtextdomain ("countries", translation_path);
        bind_textdomain_codeset ("countries", "UTF-8");

        g_free (translation_path);
      }

      {
        gchar *translation_path = g_build_filename (install_dirname, "resources", "translations", NULL);

        bindtextdomain ("BellePoule", translation_path);
        bind_textdomain_codeset ("BellePoule", "UTF-8");

        g_free (translation_path);
      }

      textdomain ("BellePoule");
    }

    g_free (install_dirname);
  }

  WifiCode::SetPort (35832);

  Gpio::Init ();

  screen = new Screen ();
  screen->ManageScoringMachine (new SgMachine ());

  gtk_main ();

  screen->Release ();

  return 0;
}
