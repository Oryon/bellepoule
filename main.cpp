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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

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
  ShellExecuteA (NULL,
                 "open",
                 link,
                 NULL,
                 NULL,
                 SW_SHOWNORMAL);
}

// --------------------------------------------------------------------------------
int main (int argc, char **argv)
{
  libintl_bindtextdomain ("BellePoule", "./resources/translations");
  libintl_bind_textdomain_codeset ("BellePoule", "UTF-8");
  textdomain ("BellePoule");

  // Init
  {
    gtk_init (&argc, &argv);
    // g_mem_set_vtable (glib_mem_profiler_table);

    Contest_c::Init             ();
    Checkin::Init               ();
    PoolAllocator_c::Init       ();
    PoolSupervisor_c::Init      ();
    Table::Init                 ();
    GeneralClassification::Init ();
    Splitting::Init             ();
  }

  Glade_c::SetPath (g_path_get_dirname (argv[0]));

  gtk_about_dialog_set_url_hook (AboutDialogActivateLinkFunc,
                                 NULL,
                                 NULL);

  {
    AttributeDesc *desc;

    desc = AttributeDesc::Declare (G_TYPE_STRING, "name");

    desc = AttributeDesc::Declare (G_TYPE_STRING, "first_name");

    desc = AttributeDesc::Declare (G_TYPE_STRING, "club");
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;

    desc = AttributeDesc::Declare (G_TYPE_INT, "rating");

    desc = AttributeDesc::Declare (G_TYPE_STRING, "gender");
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;

    desc = AttributeDesc::Declare (G_TYPE_STRING, "country");
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;

    desc = AttributeDesc::Declare (G_TYPE_STRING, "birth_year");

    desc = AttributeDesc::Declare (G_TYPE_STRING, "licence");

    desc = AttributeDesc::Declare (G_TYPE_BOOLEAN, "attending");
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;

    desc = AttributeDesc::Declare (G_TYPE_INT, "rank");
    desc->_rights = AttributeDesc::PRIVATE;

    desc = AttributeDesc::Declare (G_TYPE_INT, "ref");
    desc->_rights = AttributeDesc::PRIVATE;

    desc = AttributeDesc::Declare (G_TYPE_BOOLEAN, "exported");
    desc->_rights = AttributeDesc::PRIVATE;
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
