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

  {
    AttributeDesc::Declare (G_TYPE_STRING,   "name",       TRUE);
    AttributeDesc::Declare (G_TYPE_STRING,   "first_name", TRUE);
    AttributeDesc::Declare (G_TYPE_STRING,   "club",       FALSE);
    AttributeDesc::Declare (G_TYPE_INT,      "rating",     TRUE);
    AttributeDesc::Declare (G_TYPE_STRING,   "gender",     FALSE);
    AttributeDesc::Declare (G_TYPE_STRING,   "country",    FALSE);
    AttributeDesc::Declare (G_TYPE_STRING,   "birth_year", TRUE);
    AttributeDesc::Declare (G_TYPE_STRING,   "licence",    TRUE);

    AttributeDesc::Declare (G_TYPE_BOOLEAN,  "attending", FALSE);
    AttributeDesc::Declare (G_TYPE_INT,      "rank",      TRUE);
    AttributeDesc::Declare (G_TYPE_INT,      "ref",       TRUE);
    AttributeDesc::Declare (G_TYPE_BOOLEAN,  "exported",  FALSE);
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
