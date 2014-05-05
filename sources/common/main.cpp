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

#ifdef WINDOWS_TEMPORARY_PATCH
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

#include <gtk/gtk.h>
#include "locale"

#include "util/attribute.hpp"
#include "util/glade.hpp"
#include "people_management/barrage.hpp"
#include "people_management/checkin_supervisor.hpp"
#include "people_management/general_classification.hpp"
#include "people_management/splitting.hpp"
#include "people_management/fencer.hpp"
#include "people_management/referee.hpp"
#include "people_management/team.hpp"
#include "pool_round/pool_allocator.hpp"
#include "pool_round/pool_supervisor.hpp"
#include "table_round/table_supervisor.hpp"
#include "tournament.hpp"
#include "contest.hpp"

// --------------------------------------------------------------------------------
static void AboutDialogActivateLinkFunc (GtkAboutDialog *about,
                                         const gchar    *link,
                                         gpointer        data)
{
#ifdef WINDOWS_TEMPORARY_PATCH
  ShellExecuteA (NULL,
                 "open",
                 link,
                 NULL,
                 NULL,
                 SW_SHOWNORMAL);
#else
  gtk_show_uri (NULL,
                link,
                GDK_CURRENT_TIME,
                NULL);
#endif
}

// --------------------------------------------------------------------------------
static gint CompareRanking (Attribute *attr_a,
                            Attribute *attr_b)
{
  gint value_a = 0;
  gint value_b = 0;

  if (attr_a)
  {
    value_a = attr_a->GetIntValue ();
  }
  if (attr_b)
  {
    value_b = attr_b->GetIntValue ();
  }

  if (value_a == 0)
  {
    value_a = G_MAXINT;
  }
  if (value_b == 0)
  {
    value_b = G_MAXINT;
  }

  return (value_a - value_b);
}

// --------------------------------------------------------------------------------
static gint CompareDate (Attribute *attr_a,
                         Attribute *attr_b)
{
  GDate date_a;
  GDate date_b;

  if (attr_a)
  {
    g_date_set_parse (&date_a,
                      attr_a->GetStrValue ());
  }

  if (attr_b)
  {
    g_date_set_parse (&date_b,
                      attr_b->GetStrValue ());
  }

  return g_date_compare (&date_a,
                         &date_b);
}
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
static gboolean IsCsvReady (AttributeDesc *desc)
{
  return (   (desc->_scope       == AttributeDesc::GLOBAL)
          && (desc->_rights      == AttributeDesc::PUBLIC)
          && (desc->_persistency == AttributeDesc::PERSISTENT)
          && (g_ascii_strcasecmp (desc->_code_name, "final_rank") != 0)
          && (g_ascii_strcasecmp (desc->_code_name, "IP") != 0)
          && (g_ascii_strcasecmp (desc->_code_name, "exported")   != 0));
}

// --------------------------------------------------------------------------------
int main (int argc, char **argv)
{
  // g_mem_set_vtable (glib_mem_profiler_table);

  g_thread_init (NULL);

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

#ifdef DEBUG
      g_log_set_default_handler (LogHandler,
                                 NULL);
      install_dirname = g_build_filename (binary_dir, "..", "..", NULL);
#else
      install_dirname = g_build_filename (binary_dir, "..", "share", "BellePoule", NULL);
#endif

      g_free (binary_dir);
    }

    if (install_dirname)
    {
      gchar *gtkrc = g_build_filename (install_dirname, "resources", "gtkrc", NULL);

      gtk_rc_add_default_file (gtkrc);
      g_free (gtkrc);
    }

    {
      gtk_init (&argc, &argv);

      g_type_class_unref (g_type_class_ref (GTK_TYPE_IMAGE_MENU_ITEM));
      g_object_set (gtk_settings_get_default (), "gtk-menu-images", TRUE, NULL);
    }

    //Object::Track ("Player");

    {
      Object::SetProgramPath (install_dirname);

      Tournament::Init ();

      {
        gchar *user_language = Tournament::GetUserLanguage ();

        setlocale (LC_ALL, "");

        if (user_language)
        {
          g_setenv ("LANGUAGE",
                    user_language,
                    TRUE);
        }
      }

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

    Contest::Init ();

    {
      People::CheckinSupervisor::Declare     ();
      Pool::Allocator::Declare               ();
      Pool::Supervisor::Declare              ();
      Table::Supervisor::Declare             ();
      People::Barrage::Declare               ();
      People::GeneralClassification::Declare ();
      People::Splitting::Declare             ();
    }

    {
      Fencer::RegisterPlayerClass  ();
      Team::RegisterPlayerClass    ();
      Referee::RegisterPlayerClass ();
    }

    g_free (install_dirname);
  }

  gtk_about_dialog_set_url_hook (AboutDialogActivateLinkFunc,
                                 NULL,
                                 NULL);

  {
    AttributeDesc *desc;

    desc = AttributeDesc::Declare (G_TYPE_INT, "ref", "ID", gettext ("ref"));
    desc->_rights = AttributeDesc::PRIVATE;

    desc = AttributeDesc::Declare (G_TYPE_INT, "final_rank", "Classement", gettext ("place"));

    desc = AttributeDesc::Declare (G_TYPE_STRING, "team", "Equipe", gettext ("team"));
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;
    desc->AddDiscreteValues (NULL, NULL, NULL, NULL);

    desc = AttributeDesc::Declare (G_TYPE_STRING, "name", "Nom", gettext ("last name"));

    desc = AttributeDesc::Declare (G_TYPE_STRING, "first_name", "Prenom", gettext ("first name"));

    desc = AttributeDesc::Declare (G_TYPE_STRING, "birth_date", "DateNaissance", gettext ("date of birth"));
    desc->_compare_func = (GCompareFunc) CompareDate;

    desc = AttributeDesc::Declare (G_TYPE_STRING, "gender", "Sexe", gettext ("gender"));
    desc->_uniqueness         = AttributeDesc::NOT_SINGULAR;
    desc->_free_value_allowed = FALSE;
    desc->_favorite_look      = AttributeDesc::GRAPHICAL;
    desc->AddDiscreteValues ("M", gettext ("Male"), "resources/glade/male.png",
                             "F", gettext ("Female"), "resources/glade/female.png", NULL);

    desc = AttributeDesc::Declare (G_TYPE_STRING, "country", "Nation", gettext ("country"));
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;
    desc->_favorite_look = AttributeDesc::GRAPHICAL;
    desc->AddDiscreteValueSelector ("countries");
    AttributeDesc::AddSwappable (desc);

    desc = AttributeDesc::Declare (G_TYPE_STRING, "league", "Ligue", gettext ("league"));
    desc->_favorite_look = AttributeDesc::GRAPHICAL;
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;
    desc->AddLocalizedDiscreteValues ("ligues");
    AttributeDesc::AddSwappable (desc);

    desc = AttributeDesc::Declare (G_TYPE_STRING, "club", "Club", gettext ("club"));
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;
    desc->AddLocalizedDiscreteValues ("clubs");
    AttributeDesc::AddSwappable (desc);

    desc = AttributeDesc::Declare (G_TYPE_STRING, "licence", "Licence", gettext ("licence"));

    desc = AttributeDesc::Declare (G_TYPE_STRING, "IP", "IP", gettext ("IP address"));
    desc->_rights = AttributeDesc::PRIVATE;

    desc = AttributeDesc::Declare (G_TYPE_INT, "participation_rate", "Activite", gettext ("rate"));
    desc->_persistency    = AttributeDesc::NOT_PERSISTENT;
    desc->_favorite_look  = AttributeDesc::GRAPHICAL;
    desc->_rights         = AttributeDesc::PRIVATE;

    desc = AttributeDesc::Declare (G_TYPE_STRING, "level", "Categorie", gettext ("level"));
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;

    desc = AttributeDesc::Declare (G_TYPE_INT, "ranking", "Ranking", gettext ("ranking"));
    desc->_compare_func = (GCompareFunc) CompareRanking;

    desc = AttributeDesc::Declare (G_TYPE_BOOLEAN, "attending", "Presence", gettext ("presence"));
    desc->_uniqueness    = AttributeDesc::NOT_SINGULAR;
    desc->_favorite_look = AttributeDesc::GRAPHICAL;
    desc->_persistency   = AttributeDesc::NOT_PERSISTENT;

    desc = AttributeDesc::Declare (G_TYPE_BOOLEAN, "exported", "Exporte", gettext ("exported"));
    desc->_favorite_look = AttributeDesc::GRAPHICAL;

    desc = AttributeDesc::Declare (G_TYPE_STRING, "global_status", "Statut", gettext ("global status"));
    desc->_scope  = AttributeDesc::GLOBAL;
    desc->_rights = AttributeDesc::PRIVATE;
    desc->_favorite_look = AttributeDesc::GRAPHICAL;
    desc->AddDiscreteValues ("Q", gettext ("Qualified"),     "resources/glade/normal.png",
                             "N", gettext ("Not qualified"), "resources/glade/exit.png",
                             "A", gettext ("Withdrawal"),    "resources/glade/ambulance.png",
                             "E", gettext ("Excluded"),      "resources/glade/black_card.png",
                             "F", gettext ("Forfeit"),       "resources/glade/normal.png", NULL);

    desc = AttributeDesc::Declare (G_TYPE_STRING, "status", "Statut", gettext ("status"));
    desc->_scope = AttributeDesc::LOCAL;
    desc->_favorite_look = AttributeDesc::GRAPHICAL;
    desc->AddDiscreteValues ("Q", gettext ("Qualified"),     "resources/glade/normal.png",
                             "N", gettext ("Not qualified"), "resources/glade/exit.png",
                             "A", gettext ("Withdrawal"),    "resources/glade/ambulance.png",
                             "E", gettext ("Excluded"),      "resources/glade/black_card.png",
                             "F", gettext ("Forfeit"),       "resources/glade/normal.png", NULL);

    // Not persistent data
    {
      desc = AttributeDesc::Declare (G_TYPE_INT, "pool_nr", "pool_nr", gettext ("pool #"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_INT, "victories_ratio", "victories_ratio", gettext ("Vict./Bouts (‰)"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_INT, "indice", "indice", gettext ("index"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_INT, "HS", "HS", gettext ("Hits scored"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_INT, "rank", "rank", gettext ("place"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_rights      = AttributeDesc::PRIVATE;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_INT, "stage_start_rank", "stage_start_rank", gettext ("Round start rank"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_rights      = AttributeDesc::PRIVATE;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_BOOLEAN, "promoted", "promoted", gettext ("promoted"));
      desc->_persistency   = AttributeDesc::NOT_PERSISTENT;
      desc->_scope         = AttributeDesc::LOCAL;
      desc->_favorite_look = AttributeDesc::GRAPHICAL;

      desc = AttributeDesc::Declare (G_TYPE_STRING, "availability", "Disponibilite", gettext ("availability"));
      desc->_persistency    = AttributeDesc::NOT_PERSISTENT;
      desc->_rights         = AttributeDesc::PRIVATE;
      desc->_scope          = AttributeDesc::GLOBAL;
      desc->_favorite_look  = AttributeDesc::GRAPHICAL;
      desc->AddDiscreteValues ("Busy",   gettext ("Busy"),   (gchar *) GTK_STOCK_EXECUTE,
                               "Absent", gettext ("Absent"), (gchar *) GTK_STOCK_CLOSE,
                               "Free",   gettext ("Free"),   (gchar *) GTK_STOCK_APPLY, NULL);

      desc = AttributeDesc::Declare (G_TYPE_STRING, "connection", "Connection", gettext ("connection"));
      desc->_persistency    = AttributeDesc::NOT_PERSISTENT;
      desc->_rights         = AttributeDesc::PRIVATE;
      desc->_scope          = AttributeDesc::GLOBAL;
      desc->_favorite_look  = AttributeDesc::GRAPHICAL;
      desc->AddDiscreteValues ("Broken",  gettext ("Broken"),  "resources/glade/red.png",
                               "Waiting", gettext ("Waiting"), "resources/glade/orange.png",
                               "OK",      gettext ("OK"),      "resources/glade/green.png",
                               "Manual",  gettext ("Manual"),  (gchar *) GTK_STOCK_EDIT, NULL);

    }
  }

  AttributeDesc::SetCriteria ("CSV ready",
                              IsCsvReady);

  {
    Tournament *tournament;

    if (argc > 1)
    {
      tournament = new Tournament (g_strdup (argv[1]));
    }
    else
    {
      tournament = new Tournament (NULL);
    }

    People::Splitting::SetHostTournament (tournament);
  }

  gtk_main ();

  {
    Contest::Cleanup       ();
    AttributeDesc::Cleanup ();
  }

  return 0;
}
