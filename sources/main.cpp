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
  gchar *path = g_path_get_dirname (argv[0]);

  {
#ifdef DEBUG
    gchar *translation_path = g_strdup_printf ("%s/../../resources/translations", path);
#else
    gchar *translation_path = g_strdup_printf ("%s/resources/translations", path);
#endif

    bindtextdomain ("BellePoule", translation_path);
    bind_textdomain_codeset ("BellePoule", "UTF-8");
    textdomain ("BellePoule");

    g_free (translation_path);
  }

  // Init
  {
    // g_mem_set_vtable (glib_mem_profiler_table);

    gchar *prg_name = g_get_prgname ();

    if (prg_name)
    {
      gchar *install_dirname = g_path_get_dirname (prg_name);
      gchar *gtkrc;

      gtkrc = g_strdup_printf ("%s/resources/gtkrc", install_dirname);
      gtk_rc_add_default_file (gtkrc);
      g_free (gtkrc);
    }

    gtk_init (&argc, &argv);

    Contest::Init               ();
    Checkin::Init               ();
    PoolAllocator::Init         ();
    PoolSupervisor::Init        ();
    Table::Init                 ();
    GeneralClassification::Init ();
    Splitting::Init             ();
  }

  Glade::SetPath (path);

  gtk_about_dialog_set_url_hook (AboutDialogActivateLinkFunc,
                                 NULL,
                                 NULL);

  {
    AttributeDesc *desc;

    desc = AttributeDesc::Declare (G_TYPE_INT, "ref", "ID", "ref");
    desc->_rights = AttributeDesc::PRIVATE;

    desc = AttributeDesc::Declare (G_TYPE_STRING, "name", "Nom", "nom");

    desc = AttributeDesc::Declare (G_TYPE_STRING, "first_name", "Prenom", "prénom");

    desc = AttributeDesc::Declare (G_TYPE_STRING, "birth_date", "DateNaissance", "date naissance");
    desc->_compare_func = (GCompareFunc) CompareDate;

    desc = AttributeDesc::Declare (G_TYPE_STRING, "gender", "Sexe", "sexe");
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;
    desc->_free_value_allowed = FALSE;
    desc->AddDiscreteValues ("M", "Masculin",
                             "F", "Féminin", NULL);

    desc = AttributeDesc::Declare (G_TYPE_STRING, "country", "Nation", "nation");
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;
    desc->AddDiscreteValues ("resources/ioc_countries.txt");

    desc = AttributeDesc::Declare (G_TYPE_STRING, "ligue", "Ligue", "ligue");
    desc->AddDiscreteValues ("resources/ligues.txt");
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;

    desc = AttributeDesc::Declare (G_TYPE_STRING, "club", "Club", "club");
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;
    desc->AddDiscreteValues ("resources/clubs_fra.txt");

    desc = AttributeDesc::Declare (G_TYPE_STRING, "licence", "Licence", "licence");

    desc = AttributeDesc::Declare (G_TYPE_INT, "rating", "rating", "points");
    desc->_compare_func = (GCompareFunc) CompareRating;

    desc = AttributeDesc::Declare (G_TYPE_BOOLEAN, "attending", "attending", "présence");
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;

    desc = AttributeDesc::Declare (G_TYPE_BOOLEAN, "exported", "exported", "exporté");

    // Not persistent data
    {
      desc = AttributeDesc::Declare (G_TYPE_INT, "victories_ratio", "victories_ratio", "Vict./Matchs (‰)");
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_INT, "indice", "indice", "indice");
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_INT, "HS", "HS", "Touches données");
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_INT, "rank", "rank", "place");
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_rights      = AttributeDesc::PRIVATE;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_INT, "previous_stage_rank", "previous_stage_rank", "rang d'entrée");
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
