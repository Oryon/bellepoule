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

#define GETTEXT_PACKAGE "gtk20"
#include <glib/gi18n-lib.h>

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

#include "contest.hpp"
#include "checkin.hpp"
#include "pool_allocator.hpp"
#include "pool_supervisor.hpp"
#include "table.hpp"
#include "splitting.hpp"
#include "tournament.hpp"
#include "attribute.hpp"
#include "general_classification.hpp"
#include "glade.hpp"

// --------------------------------------------------------------------------------
static void AboutDialogActivateLinkFunc (GtkAboutDialog *about,
                                         const gchar    *link,
                                         gpointer        data)
{
#ifdef WINDOWS_TEMPORARY_PATCH
  g_print (">>>>>>>>>>>>>>>>>>>>>>\n");
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
static gint CompareRating (Attribute *attr_a,
                           Attribute *attr_b)
{
  gint value_a = 0;
  gint value_b = 0;

  if (attr_a)
  {
    value_a = (gint) attr_a->GetValue ();
  }
  if (attr_b)
  {
    value_b = (gint) attr_b->GetValue ();
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
                      (const gchar *) attr_a->GetValue ());
  }
  if (attr_b)
  {
    g_date_set_parse (&date_b,
                      (const gchar *) attr_b->GetValue ());
  }

  return g_date_compare (&date_a,
                         &date_b);
}

// --------------------------------------------------------------------------------
int main (int argc, char **argv)
{
  // Init
  {
    gchar *install_dirname = g_get_current_dir ();
    // g_mem_set_vtable (glib_mem_profiler_table);

    if (install_dirname)
    {
      gchar *gtkrc = g_build_filename (install_dirname, "resources", "gtkrc", NULL);

      gtk_rc_add_default_file (gtkrc);
      g_free (gtkrc);
    }

    gtk_init (&argc, &argv);

    {
      gchar  *translation_path = g_build_filename (install_dirname, "resources", "translations", NULL);

      setlocale (LC_ALL, "");

      //g_setenv ("LANGUAGE",
      //"en",
      //TRUE);

      bindtextdomain ("BellePoule", translation_path);
      bind_textdomain_codeset ("BellePoule", "UTF-8");
      textdomain ("BellePoule");

      g_free (translation_path);
    }

    Contest::Init               ();
    Checkin::Init               ();
    PoolAllocator::Init         ();
    PoolSupervisor::Init        ();
    Table::Init                 ();
    GeneralClassification::Init ();
    Splitting::Init             ();

    Glade::SetPath (install_dirname);

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

    desc = AttributeDesc::Declare (G_TYPE_STRING, "name", "Nom", gettext ("nom"));

    desc = AttributeDesc::Declare (G_TYPE_STRING, "first_name", "Prenom", gettext ("prénom"));

    desc = AttributeDesc::Declare (G_TYPE_STRING, "birth_date", "DateNaissance", gettext ("date naissance"));
    desc->_compare_func = (GCompareFunc) CompareDate;

    desc = AttributeDesc::Declare (G_TYPE_STRING, "gender", "Sexe", gettext ("sexe"));
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;
    desc->_free_value_allowed = FALSE;
    desc->AddDiscreteValues ("M", gettext ("Masculin"),
                             "F", gettext ("Féminin"), NULL);

    desc = AttributeDesc::Declare (G_TYPE_STRING, "country", "Nation", gettext ("nation"));
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;
    desc->AddDiscreteValues ("resources/ioc_countries.txt");

    desc = AttributeDesc::Declare (G_TYPE_STRING, "ligue", "Ligue", gettext ("ligue"));
    desc->AddDiscreteValues ("resources/ligues.txt");
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;

    desc = AttributeDesc::Declare (G_TYPE_STRING, "club", "Club", gettext ("club"));
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;
    desc->AddDiscreteValues ("resources/clubs_fra.txt");

    desc = AttributeDesc::Declare (G_TYPE_STRING, "licence", "Licence", gettext ("licence"));

    desc = AttributeDesc::Declare (G_TYPE_INT, "rating", "Classement", gettext ("classement"));
    desc->_compare_func = (GCompareFunc) CompareRating;

    desc = AttributeDesc::Declare (G_TYPE_BOOLEAN, "attending", "Presence", gettext ("présence"));
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;

    desc = AttributeDesc::Declare (G_TYPE_BOOLEAN, "exported", "Exporte", gettext ("exporté"));

    // Not persistent data
    {
      desc = AttributeDesc::Declare (G_TYPE_INT, "victories_ratio", "victories_ratio", gettext ("Vict./Matchs (‰)"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_INT, "indice", "indice", gettext ("indice"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_INT, "HS", "HS", gettext ("Touches données"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_INT, "rank", "rank", gettext ("place"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_rights      = AttributeDesc::PRIVATE;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_INT, "previous_stage_rank", "previous_stage_rank", gettext ("rang d'entrée"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_rights      = AttributeDesc::PRIVATE;
      desc->_scope       = AttributeDesc::LOCAL;
    }
  }

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

    Splitting::SetHostTournament (tournament);
  }

  gtk_main ();

  return 0;
}
