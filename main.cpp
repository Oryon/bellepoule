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

    Contest_c::Init        ();
    Checkin::Init          ();
    PoolAllocator_c::Init  ();
    PoolSupervisor_c::Init ();
    Table::Init            ();
    Splitting::Init        ();
  }

  {
    Attribute_c::Add (G_TYPE_INT,     "ref");
    Attribute_c::Add (G_TYPE_STRING,  "name");
    Attribute_c::Add (G_TYPE_STRING,  "first_name");
    Attribute_c::Add (G_TYPE_STRING,  "gender");
    Attribute_c::Add (G_TYPE_STRING,  "country");
    Attribute_c::Add (G_TYPE_INT,     "rating");
    Attribute_c::Add (G_TYPE_INT,     "rank");
    Attribute_c::Add (G_TYPE_BOOLEAN, "attending");
    Attribute_c::Add (G_TYPE_STRING,  "club");
    Attribute_c::Add (G_TYPE_STRING,  "birth_year");
    Attribute_c::Add (G_TYPE_STRING,  "licence");
  }

  {
    Tournament *tournament = new Tournament ();

    Splitting::SetHostTournament (tournament);
  }

  gtk_main ();

  return 0;
}
