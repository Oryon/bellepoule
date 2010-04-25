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

#include "version.h"
#include "contest.hpp"

#include "tournament.hpp"

// --------------------------------------------------------------------------------
Tournament::Tournament (gchar *filename)
  : Module ("tournament.glade")
{
  _contest_list = NULL;

  ReadConfiguration ();

  // Show the main window
  {
    GtkWidget *w = _glade->GetRootWidget ();

    gtk_widget_show_all (w);
    gtk_widget_hide (_glade->GetWidget ("notebook"));
  }

  if (filename)
  {
    Contest *contest;

    contest = new Contest (filename);
    Manage (contest);
    g_free (filename);
  }

  {
    GtkWidget *w       = _glade->GetWidget ("about_dialog");
    gchar     *version = g_strdup_printf ("%s.%s/%s", VERSION, VERSION_DAY, VERSION_MONTH);

    gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (w),
                                  (const gchar *) version);

    g_free (version);
  }
}

// --------------------------------------------------------------------------------
Tournament::~Tournament ()
{
  FILE *file;

  {
    gchar *file_path = g_strdup_printf ("%s/BellePoule/config.ini", g_get_user_config_dir ());

    file = g_fopen (file_path,
                    "w");
    g_free (file_path);
  }

  if (file)
  {
    gsize  length;
    gchar *config = g_key_file_to_data (_config_file,
                                        &length,
                                        NULL);

    fwrite (config, sizeof (char), strlen (config), file);
    g_free (config);

    fclose (file);
  }

  g_key_file_free (_config_file);
}

// --------------------------------------------------------------------------------
void Tournament::ReadConfiguration ()
{
  gchar *dir_path  = g_strdup_printf ("%s/BellePoule", g_get_user_config_dir ());
  gchar *file_path = g_strdup_printf ("%s/config.ini", dir_path);

  _config_file = g_key_file_new ();
  if (g_key_file_load_from_file (_config_file,
                                 file_path,
                                 G_KEY_FILE_KEEP_COMMENTS,
                                 NULL) == FALSE)
  {
    g_mkdir_with_parents (dir_path,
                          0700);

    g_key_file_set_string (_config_file,
                           "Competiton",
                           "default_dir_name",
                           g_get_user_special_dir (G_USER_DIRECTORY_DOCUMENTS));

    g_key_file_set_string (_config_file,
                           "Checkin",
                           "default_import_dir_name",
                           g_get_user_special_dir (G_USER_DIRECTORY_DOCUMENTS));
  }

  g_free (dir_path);
  g_free (file_path);
}

// --------------------------------------------------------------------------------
void Tournament::Manage (Contest *contest)
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
void Tournament::OnContestDeleted (Contest *contest)
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
Contest *Tournament::GetContest (gchar *filename)
{
  for (guint i = 0; i < g_slist_length (_contest_list); i++)
  {
    Contest *contest;

    contest = (Contest *) g_slist_nth_data (_contest_list,
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
  Contest *contest = Contest::Create ();

  Manage (contest);
}

// --------------------------------------------------------------------------------
void Tournament::OnOpen (gchar *current_folder)
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

  if (current_folder && (g_file_test (current_folder,
                                      G_FILE_TEST_IS_DIR)))
  {
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser),
                                         current_folder);
  }
  else
  {
    gchar *last_dirname = g_key_file_get_string (_config_file,
                                                 "Competiton",
                                                 "default_dir_name",
                                                 NULL);
    if (last_dirname && (g_file_test (last_dirname,
                                      G_FILE_TEST_IS_DIR)))
    {
      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser),
                                           last_dirname);

      g_free (last_dirname);
    }
  }

  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
  {
    gchar *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));

    if (filename)
    {
      {
        gchar *dirname = g_path_get_dirname (filename);

        g_key_file_set_string (_config_file,
                               "Competiton",
                               "default_dir_name",
                               dirname);
        g_free (dirname);
      }

      {
        Contest *contest = new Contest (filename);

        Manage (contest);
      }

      g_free (filename);
    }
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
void Tournament::OnOpenExample ()
{
  {
    gchar *prg_name = g_get_prgname ();

    if (prg_name)
    {
      gchar *install_dirname = g_path_get_dirname (prg_name);

      if (install_dirname)
      {
        gchar *example_dirname = g_strdup_printf ("%s/Exemples_Fichiers_BellePoule", install_dirname);

        OnOpen (example_dirname);
        g_free (example_dirname);
        g_free (install_dirname);
      }
    }
  }
}

// --------------------------------------------------------------------------------
void Tournament::OnRecent ()
{
  GtkWidget *dialog = gtk_recent_chooser_dialog_new ("Fichiers récemment ouverts",
                                                     NULL,
                                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                     GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                                     NULL);

  {
    GtkRecentFilter *filter = gtk_recent_filter_new ();

    gtk_recent_filter_add_pattern (filter,
                                   "*.cotcot");
    gtk_recent_filter_set_name (filter,
                                "Fichiers BellePoule");

    gtk_recent_chooser_add_filter (GTK_RECENT_CHOOSER (dialog),
                                   filter);
  }

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    GtkRecentInfo *info;

    info = gtk_recent_chooser_get_current_item (GTK_RECENT_CHOOSER (dialog));

    if (info)
    {
      {
        gchar *dirname = g_path_get_dirname (gtk_recent_info_get_uri (info));

        g_key_file_set_string (_config_file,
                               "Competiton",
                               "default_dir_name",
                               dirname);
        g_free (dirname);
      }

      {
        Contest *contest = new Contest ((gchar *) gtk_recent_info_get_uri_display (info));

        Manage (contest);
      }

      gtk_recent_info_unref (info);
    }
  }

  gtk_widget_destroy (dialog);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_new_menuitem_activate (GtkWidget *w,
                                                          Object    *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnNew ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_open_menuitem_activate (GtkWidget *w,
                                                           Object    *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnOpen ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_about_menuitem_activate (GtkWidget *w,
                                                           Object    *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnAbout ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_example_menuitem_activate (GtkWidget *w,
                                                              Object    *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnOpenExample ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT gboolean on_root_delete_event (GtkWidget *w,
                                                          GdkEvent  *event,
                                                          Object    *owner)
{
  GtkWidget *dialog = gtk_message_dialog_new_with_markup (NULL,
                                                          GTK_DIALOG_MODAL,
                                                          GTK_MESSAGE_QUESTION,
                                                          GTK_BUTTONS_OK_CANCEL,
                                                          "<b><big>Voulez-vous vraiment quitter BellePoule ?</big></b>");

  gtk_window_set_title (GTK_WINDOW (dialog),
                        "Quitter BellePoule ?");

  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            "Toutes les compétitions non sauvegardées seront perdues.");

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
  {
    Tournament *t = dynamic_cast <Tournament *> (owner);

    t->Release ();
    gtk_main_quit ();
  }
  else
  {
    gtk_widget_destroy (dialog);
  }

  return TRUE;
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_recent_menuitem_activate (GtkWidget *w,
                                                             Object    *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnRecent ();
}