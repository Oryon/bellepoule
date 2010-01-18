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
#include <glib.h>
#include <glib/gstdio.h>

#include "contest.hpp"

#include "tournament.hpp"

// --------------------------------------------------------------------------------
Tournament::Tournament (gchar *filename)
  : Module_c ("tournament.glade")
{
  _contest_list = NULL;

  // Show the main window
  {
    GtkWidget *w = _glade->GetRootWidget ();

    gtk_widget_show_all (w);
    gtk_widget_hide (_glade->GetWidget ("notebook"));
  }

#ifdef DEBUG
  if (filename == NULL)
  {
    gchar     *current_dir = g_get_current_dir ();

    filename = g_build_filename (current_dir,
                                 "Exemples_Fichiers_BellePoule", "minimes_bretagne.cotcot",
                                 NULL);
    g_free (current_dir);
  }
#endif

  if (filename)
  {
    Contest_c *contest;

    contest = new Contest_c (filename);
    Manage (contest);
    g_free (filename);
  }

  {
    //GtkWidget *w = _glade->GetWidget ("about_dialog");

    //gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (w),
                                  //(const gchar *) ">> 0.17/01 <<");
  }
}

// --------------------------------------------------------------------------------
Tournament::~Tournament ()
{
}

// --------------------------------------------------------------------------------
void Tournament::Manage (Contest_c *contest)
{
  if (contest)
  {
    GtkWidget *nb = _glade->GetWidget ("notebook");

    contest->AttachTo (GTK_NOTEBOOK (nb));
    contest->SetTournament (this);

    _contest_list = g_slist_append (_contest_list,
                                    contest);
    if (g_slist_length (_contest_list) == 1)
    {
      gtk_widget_show (_glade->GetWidget ("notebook"));
      gtk_widget_hide (_glade->GetWidget ("logo"));
    }
  }
}

// --------------------------------------------------------------------------------
void Tournament::OnContestDeleted (Contest_c *contest)
{
  if (_contest_list)
  {
    _contest_list = g_slist_remove (_contest_list,
                                    contest);
    if (g_slist_length (_contest_list) == 0)
    {
      gtk_widget_show (_glade->GetWidget ("logo"));
      gtk_widget_hide (_glade->GetWidget ("notebook"));
    }
  }
}

// --------------------------------------------------------------------------------
Contest_c *Tournament::GetContest (gchar *filename)
{
  for (guint i = 0; i < g_slist_length (_contest_list); i++)
  {
    Contest_c *contest;

    contest = (Contest_c *) g_slist_nth_data (_contest_list,
                                              i);
    if (strcmp (filename, contest->GetFilename ()) == 0)
    {
      return contest;
    }
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void Tournament::OnNew ()
{
  // g_mem_profile ();
  Contest_c *contest = Contest_c::Create ();

  Manage (contest);
  // g_mem_profile ();
}

// --------------------------------------------------------------------------------
void Tournament::OnOpen ()
{
  GtkWidget *chooser = gtk_file_chooser_dialog_new (gettext ("Choose a competition file to open..."),
                                                    NULL,
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    GTK_STOCK_CANCEL,
                                                    GTK_RESPONSE_CANCEL,
                                                    GTK_STOCK_OPEN,
                                                    GTK_RESPONSE_ACCEPT,
                                                    NULL);

  {
    GtkFileFilter *filter = gtk_file_filter_new ();

    gtk_file_filter_set_name (filter,
                              "All BellePoule files (.cocot)");
    gtk_file_filter_add_pattern (filter,
                                 "*.cotcot");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser),
                                 filter);
  }

  {
    GtkFileFilter *filter = gtk_file_filter_new ();

    gtk_file_filter_set_name (filter,
                              "All files");
    gtk_file_filter_add_pattern (filter,
                                 "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser),
                                 filter);
  }

  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
  {
    Contest_c *contest;
    gchar     *filename;

    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
    contest = new Contest_c (filename);
    Manage (contest);
  }

  gtk_widget_destroy (chooser);
}

// --------------------------------------------------------------------------------
void Tournament::OnAbout ()
{
  GtkWidget *w = _glade->GetWidget ("about_dialog");

  gtk_dialog_run (GTK_DIALOG (w));

  gtk_widget_hide (w);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_main_window_delete_event (GtkWidget *w,
                                                             gpointer   data)
{
  gtk_main_quit ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_new_menuitem_activate (GtkWidget *w,
                                                          Object_c  *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnNew ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_open_menuitem_activate (GtkWidget *w,
                                                           Object_c  *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnOpen ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_about_menuitem_activate (GtkWidget *w,
                                                           Object_c  *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnAbout ();
}
