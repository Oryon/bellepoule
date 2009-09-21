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
#include "contest.hpp"
#include "pool_allocator.hpp"
#include "pool_supervisor.hpp"
#include "attribute.hpp"

static Glade_c *xml = NULL;

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_main_window_delete_event (GtkWidget *w,
                                                             gpointer   data)
{
  gtk_main_quit ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_new_menuitem_activate (GtkWidget *w,
                                                          gpointer   data)
{
  GtkWidget *nb = xml->GetWidget ("notebook");
  Contest_c *contest;

  contest = Contest_c::Create ();
  if (contest)
  {
    contest->AttachTo (GTK_NOTEBOOK (nb));
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_open_menuitem_activate (GtkWidget *w,
                                                           gpointer   data)
{
  GtkWidget *chooser;

  chooser = GTK_WIDGET (gtk_file_chooser_dialog_new ("Choisissez un fichier ...",
                                                     NULL,
                                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                                     GTK_STOCK_CANCEL,
                                                     GTK_RESPONSE_CANCEL,
                                                     GTK_STOCK_OPEN,
                                                     GTK_RESPONSE_ACCEPT,
                                                     NULL));

  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
  {
    Contest_c *contest;
    gchar     *filename;

    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
    contest = new Contest_c (filename);
    contest->AttachTo (GTK_NOTEBOOK (xml->GetWidget ("notebook")));
  }

  gtk_widget_destroy (chooser);
}

// --------------------------------------------------------------------------------
int main (int argc, char **argv)
{
  // Init
  {
    gtk_set_locale  ();
    gtk_init        (&argc, &argv);
    glade_init      ();

    Contest_c::Init        ();
    PoolAllocator_c::Init  ();
    PoolSupervisor_c::Init ();
  }

  {
    Attribute_c::Add (G_TYPE_INT,     "ref");
    Attribute_c::Add (G_TYPE_STRING,  "name");
    Attribute_c::Add (G_TYPE_STRING,  "first_name");
    Attribute_c::Add (G_TYPE_INT,     "rating");
    Attribute_c::Add (G_TYPE_INT,     "rank");
    Attribute_c::Add (G_TYPE_BOOLEAN, "attending");
    Attribute_c::Add (G_TYPE_STRING,  "club");
    Attribute_c::Add (G_TYPE_STRING,  "birth_year");
    Attribute_c::Add (G_TYPE_STRING,  "licence");
  }

  xml = new Glade_c ("main_frame.glade");

  {
    Contest_c *contest;
    gchar     *filename = "compet/minimes_bretagne";

    contest = new Contest_c (filename);
    contest->AttachTo (GTK_NOTEBOOK (xml->GetWidget ("notebook")));
  }

  // Show the main window
  {
    GtkWidget *w = xml->GetRootWidget ();

    if (w == NULL) return 1;
    gtk_widget_show_all (w);
  }

  gtk_main ();

  return 0;
}
