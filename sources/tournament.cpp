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

#include <glib.h>
#include <glib/gstdio.h>
#include <curl/curl.h>

#ifdef WINDOWS_TEMPORARY_PATCH
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

#include "version.h"
#include "update_checker.hpp"
#include "contest.hpp"

#include "tournament.hpp"

// --------------------------------------------------------------------------------
Tournament::Tournament (gchar *filename)
  : Module ("tournament.glade")
{
  _contest_list = NULL;
  _referee_list = NULL;
  _referee_ref  = 1;

  curl_global_init (CURL_GLOBAL_ALL);

  EnumerateLanguages ();

  // Show the main window
  {
    GtkWidget *w = _glade->GetRootWidget ();

    gtk_widget_show_all (w);
    gtk_widget_hide (_glade->GetWidget ("notebook"));
  }

  gtk_widget_hide (_glade->GetWidget ("update_menuitem"));

  if (filename)
  {
    gchar *utf8_name;

    {
      gsize   bytes_written;
      GError *error = NULL;

      utf8_name = g_convert (filename,
                             -1,
                             "UTF-8",
                             "ISO-8859-1",
                             NULL,
                             &bytes_written,
                             &error);

      if (error)
      {
        g_print ("<<ConvertToUtf8>> %s\n", error->message);
        g_clear_error (&error);
      }
    }

    OpenContest (utf8_name);
    g_free (utf8_name);

    g_free (filename);
  }

  {
    GtkWidget *w       = _glade->GetWidget ("about_dialog");
    gchar     *version = g_strdup_printf ("V%s.%s%s", VERSION, VERSION_REVISION, VERSION_MATURITY);

    gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (w),
                                  (const gchar *) version);
    g_free (version);
  }

  {
    GtkWidget   *w         = _glade->GetWidget ("about_dialog");
    const gchar *authors[] = {"Florence Blanchard",
                              "Laurent Bonnot",
                              "Tony (ajs New Mexico)",
                              "Emmanuel Chaix",
                              "Julien Diaz",
                              "Olivier Larcher",
                              "Yannick Le Roux",
                              "Jean-Pierre Mahé",
                              "Pierre Moro",
                              "Killian Poulet",
                              "Michel Relet",
                              "Vincent Rémy",
                              "Tina Schliemann",
                              "Claude Simonnot",
                              "Sébastien Vermandel",
                              NULL};

    gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG (w),
                                  authors);
  }

  {
    GtkWidget *w           = _glade->GetWidget ("about_dialog");
    gchar     *translators = g_strdup_printf ("Julien Diaz       (German)\n"
                                              "Tina Schliemann   (German)\n"
                                              "Aureliano Martini (Italian)\n"
                                              "Jihwan Cho        (Korean)\n"
                                              "Marijn Somers     (Dutch)\n"
                                              "Werner Huysmans   (Dutch)\n"
                                              "Alexis Pigeon     (Spanish)\n"
                                              "Mohamed Rebai     (Arabic)\n"
                                              "Sergev Makhtanov  (Russian)");

    gtk_about_dialog_set_translator_credits (GTK_ABOUT_DIALOG (w),
                                             translators);
    g_free (translators);
  }

  {
    gchar *last_backup = g_key_file_get_string (_config_file,
                                                "Tournament",
                                                "backup_location",
                                                NULL);
    SetBackupLocation (last_backup);
    g_free (last_backup);
  }

  {
    GtkWidget     *main_window = GTK_WIDGET (_glade->GetWidget ("root"));
    GtkTargetList *target_list;

    gtk_drag_dest_set (main_window,
                       GTK_DEST_DEFAULT_ALL,
                       NULL,
                       0,
                       GDK_ACTION_COPY);

    target_list = gtk_drag_dest_get_target_list (main_window);

    if (target_list == NULL)
    {
      target_list = gtk_target_list_new (NULL, 0);
      gtk_drag_dest_set_target_list (main_window, target_list);
      gtk_target_list_unref (target_list);
    }
    gtk_target_list_add_uri_targets (target_list, 100);
  }

  UpdateChecker::DownloadLatestVersion ((GSourceFunc) OnLatestVersionReceived,
                                        this);

  _network = new Network ();
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

    // fclose (file);
  }

  g_slist_free (_referee_list);

  g_key_file_free (_config_file);

  curl_global_cleanup ();

  _network->Release ();
}

// --------------------------------------------------------------------------------
void Tournament::Init ()
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
Player *Tournament::Share (Player *referee)
{
  Player *original  = NULL;
  GSList *current   = _referee_list;
  GSList *attr_list = NULL;
  Player::AttributeId  name_attr_id       ("name");
  Player::AttributeId  first_name_attr_id ("first_name");

  attr_list = g_slist_prepend (attr_list, &first_name_attr_id);
  attr_list = g_slist_prepend (attr_list, &name_attr_id);

  while (current)
  {
    Player *current_referee = (Player *) current->data;

    if (Player::MultiCompare (referee,
                              current_referee,
                              attr_list) == 0)
    {
      original = current_referee;
      break;
    }

    current = g_slist_next (current);
  }

  g_slist_free (attr_list);

  if (original == NULL)
  {
    _referee_list = g_slist_prepend (_referee_list,
                                     referee);
    referee->SetRef (_referee_ref++);
  }

  return original;
}

// --------------------------------------------------------------------------------
void Tournament::EnumerateLanguages ()
{
  gchar  *contents;
  GSList *group        = NULL;
  gchar  *filename     = g_build_filename (_program_path, "resources", "translations", "index.txt", NULL);
  gchar  *last_setting = g_key_file_get_string (_config_file,
                                                "Tournament",
                                                "interface_language",
                                                NULL);

  if (g_file_get_contents (filename,
                           &contents,
                           NULL,
                           NULL) == TRUE)
  {
    gchar **lines = g_strsplit_set (contents, "\n", 0);

    if (lines)
    {
      const gchar *env    = g_getenv ("LANGUAGE");
      gchar *original_env = NULL;

      if (env)
      {
        original_env = g_strdup (env);
      }

      for (guint l = 0; lines[l] && lines[l][0]; l++)
      {
        gchar **tokens = g_strsplit_set (lines[l], ";", 0);

        if (tokens)
        {
          GtkWidget *item;

          g_setenv ("LANGUAGE",
                    tokens[0],
                    TRUE);

          g_strdelimit (tokens[1],
                        "\n\r",
                        '\0');

          item  = gtk_radio_menu_item_new_with_label (group, tokens[1]);
          group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));

          gtk_menu_shell_append (GTK_MENU_SHELL (_glade->GetWidget ("locale_menu")),
                                 item);
          gtk_widget_set_tooltip_markup (item,
                                         gettext ("Restart BellePoule for this change to take effect"));

          g_signal_connect (item, "toggled",
                            G_CALLBACK (OnLocaleToggled), (void *)
                            tokens[0]);
          if (last_setting && strcmp (last_setting, tokens[0]) == 0)
          {
            gtk_menu_item_set_label (GTK_MENU_ITEM (_glade->GetWidget ("locale_menuitem")),
                                     tokens[0]);
            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item),
                                            TRUE);
          }
          gtk_widget_show (item);
        }
      }

      if (original_env)
      {
        g_setenv ("LANGUAGE",
                  original_env,
                  TRUE);
        g_free (original_env);
      }
      else
      {
        g_unsetenv ("LANGUAGE");
      }
    }
  }

  g_free (filename);
}

// --------------------------------------------------------------------------------
gchar *Tournament::GetUserLanguage ()
{
  return g_key_file_get_string (_config_file,
                                "Tournament",
                                "interface_language",
                                NULL);
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
  GtkWidget *chooser = gtk_file_chooser_dialog_new (gettext ("Choose a competion file to open... "),
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
                              gettext ("BellePoule files (.cotcot)"));
    gtk_file_filter_add_pattern (filter,
                                 "*.cotcot");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser),
                                 filter);
  }

  {
    GtkFileFilter *filter = gtk_file_filter_new ();

    gtk_file_filter_set_name (filter,
                              gettext ("FFE files (.xml)"));
    gtk_file_filter_add_pattern (filter,
                                 "*.xml");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser),
                                 filter);
  }

  {
    GtkFileFilter *filter = gtk_file_filter_new ();

    gtk_file_filter_set_name (filter,
                              gettext ("All files"));
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

    OpenContest (filename);
    g_free (filename);
  }

  gtk_widget_destroy (chooser);
}

// --------------------------------------------------------------------------------
void Tournament::OnSave ()
{
  GSList *current = _contest_list;

  while (current)
  {
    Contest *contest;

    contest = (Contest *) current->data;
    contest->Save ();

    current = g_slist_next (current);
  }
}

// --------------------------------------------------------------------------------
void Tournament::OnAbout ()
{
  GtkWidget *w = _glade->GetWidget ("about_dialog");

  gtk_dialog_run (GTK_DIALOG (w));

  gtk_widget_hide (w);
}

// --------------------------------------------------------------------------------
void Tournament::OnOpenUserManual ()
{
  gchar *uri = g_build_filename (_program_path, "resources", "user_manual.pdf", NULL);

#ifdef WINDOWS_TEMPORARY_PATCH
  ShellExecute (NULL,
                "open",
                uri,
                NULL,
                NULL,
                SW_SHOWNORMAL);
#else
  gtk_show_uri (NULL,
                uri,
                GDK_CURRENT_TIME,
                NULL);
#endif

  g_free (uri);
}

// --------------------------------------------------------------------------------
void Tournament::OpenContest (const gchar *uri)
{
  if (uri)
  {
    SetCursor (GDK_WATCH);

    {
      gchar *dirname = g_path_get_dirname (uri);

      g_key_file_set_string (_config_file,
                             "Competiton",
                             "default_dir_name",
                             dirname);
      g_free (dirname);
    }

    {
      Contest *contest = new Contest (uri);

      Manage (contest);
    }

    ResetCursor ();

    while (gtk_events_pending ())
    {
      gtk_main_iteration ();
    }
  }
}

// --------------------------------------------------------------------------------
void Tournament::OnOpenExample ()
{
  gchar *prg_name = g_get_prgname ();

  if (prg_name)
  {
    gchar *install_dirname = g_path_get_dirname (prg_name);

    if (install_dirname)
    {
      gchar *example_dirname = g_strdup_printf ("%s/Exemples", install_dirname);

      OnOpen (example_dirname);
      g_free (example_dirname);
      g_free (install_dirname);
    }
  }
}

// --------------------------------------------------------------------------------
void Tournament::OnRecent ()
{
  GtkWidget *dialog = gtk_recent_chooser_dialog_new (gettext ("Recently opened files"),
                                                     NULL,
                                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                     GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                                     NULL);

  gtk_recent_chooser_set_show_tips  (GTK_RECENT_CHOOSER (dialog),
                                     TRUE);

  {
    GtkRecentFilter *filter = gtk_recent_filter_new ();

    gtk_recent_filter_add_pattern (filter,
                                   "*.cotcot");
    gtk_recent_filter_set_name (filter,
                                gettext ("BellePoule files"));

    gtk_recent_chooser_add_filter (GTK_RECENT_CHOOSER (dialog),
                                   filter);
  }

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    GtkRecentInfo *info;

    info = gtk_recent_chooser_get_current_item (GTK_RECENT_CHOOSER (dialog));

    if (info)
    {
      OpenContest (gtk_recent_info_get_uri_display (info));

      gtk_recent_info_unref (info);
    }
  }

  gtk_widget_destroy (dialog);
}

// --------------------------------------------------------------------------------
void Tournament::OnBackupfileLocation ()
{
  GtkWidget *chooser = GTK_WIDGET (gtk_file_chooser_dialog_new (gettext ("Choose a backup files location..."),
                                                                GTK_WINDOW (_glade->GetRootWidget ()),
                                                                GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                                                GTK_STOCK_CANCEL,
                                                                GTK_RESPONSE_CANCEL,
                                                                GTK_STOCK_APPLY,
                                                                GTK_RESPONSE_ACCEPT,
                                                                NULL));

  {
    gchar *last_location = g_key_file_get_string (_config_file,
                                                  "Tournament",
                                                  "backup_location",
                                                  NULL);
    if (last_location)
    {
      gtk_file_chooser_select_uri (GTK_FILE_CHOOSER (chooser),
                                   last_location);

      g_free (last_location);
    }
  }

  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
  {
    gchar *foldername = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (chooser));

    if (foldername)
    {
      g_key_file_set_string (_config_file,
                             "Tournament",
                             "backup_location",
                             foldername);
      SetBackupLocation (foldername);
      g_free (foldername);
    }
  }

  gtk_widget_destroy (chooser);
}

// --------------------------------------------------------------------------------
void Tournament::SetBackupLocation (gchar *location)
{
  if (location)
  {
    gchar *readable_name = g_filename_from_uri (location,
                                                NULL,
                                                NULL);

    gtk_menu_item_set_label (GTK_MENU_ITEM (_glade->GetWidget ("backup_location_menuitem")),
                             readable_name);
    g_free (readable_name);
  }
}

// --------------------------------------------------------------------------------
const gchar *Tournament::GetBackupLocation ()
{
  if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (_glade->GetWidget ("activate_radiomenuitem"))))
  {
    const gchar *location = gtk_menu_item_get_label (GTK_MENU_ITEM (_glade->GetWidget ("backup_location_menuitem")));

    if (g_file_test (location, G_FILE_TEST_IS_DIR))
    {
      return location;
    }
  }
  return NULL;
}

// --------------------------------------------------------------------------------
void Tournament::OnActivateBackup ()
{
  if (GetBackupLocation () == NULL)
  {
    OnBackupfileLocation ();
  }
}

// --------------------------------------------------------------------------------
void Tournament::OnLocaleToggled (GtkCheckMenuItem *checkmenuitem,
                                  gchar            *locale)
{
  if (gtk_check_menu_item_get_active (checkmenuitem))
  {
    g_key_file_set_string (_config_file,
                           "Tournament",
                           "interface_language",
                           locale);
  }
}

// --------------------------------------------------------------------------------
gboolean Tournament::OnLatestVersionReceived (Tournament *tournament)
{
  GKeyFile *version_file = g_key_file_new ();

  if (UpdateChecker::GetLatestVersion (version_file))
  {
    gchar    *version;
    gchar    *revision;
    gchar    *maturity;
    gboolean  new_version_detected = FALSE;

    version = g_key_file_get_string (version_file,
                                     VERSION_BRANCH,
                                     "version",
                                     NULL);
    revision = g_key_file_get_string (version_file,
                                      VERSION_BRANCH,
                                      "revision",
                                      NULL);
    maturity = g_key_file_get_string (version_file,
                                      VERSION_BRANCH,
                                      "maturity",
                                      NULL);

    if (version && (strcmp (VERSION, version) != 0))
    {
      new_version_detected = TRUE;
    }
    else if (revision && (strcmp (VERSION_REVISION, revision) != 0))
    {
      new_version_detected = TRUE;
    }
    else if (maturity && (strcmp (VERSION_MATURITY, maturity) != 0))
    {
      new_version_detected = TRUE;
    }

    if (new_version_detected)
    {
      gchar *label = g_strdup_printf ("%s.%s.%s", version, revision, maturity);

      gtk_menu_item_set_label (GTK_MENU_ITEM (tournament->_glade->GetWidget ("new_version_menuitem")),
                               label);
      gtk_widget_show (tournament->_glade->GetWidget ("update_menuitem"));
      g_free (label);
    }
  }

  g_key_file_free (version_file);

  return FALSE;
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
extern "C" G_MODULE_EXPORT void on_save_menuitem_activate (GtkWidget *w,
                                                           Object    *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnSave ();
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
extern "C" G_MODULE_EXPORT void on_user_manual_activate (GtkWidget *w,
                                                         Object    *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnOpenUserManual ();
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
                                                          gettext ("<b><big>Do you really want to quit BellePoule</big></b>"));

  gtk_window_set_title (GTK_WINDOW (dialog),
                        gettext ("Quit BellePoule?"));

  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            gettext ("All the unsaved competions will be lost."));

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

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_backup_location_menuitem_activate (GtkWidget *widget,
                                                                      Object    *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnBackupfileLocation ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_activate_radiomenuitem_toggled (GtkCheckMenuItem *checkmenuitem,
                                                                   Object           *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  if (gtk_check_menu_item_get_active (checkmenuitem))
  {
    t->OnActivateBackup ();
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_root_drag_data_received (GtkWidget        *widget,
                                                            GdkDragContext   *context,
                                                            gint              x,
                                                            gint              y,
                                                            GtkSelectionData *selection_data,
                                                            guint             info,
                                                            guint             timestamp,
                                                            Object           *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  if (info == 100)
  {
    gchar **uris = g_uri_list_extract_uris ((gchar *) gtk_selection_data_get_data (selection_data));

    for (guint i = 0; uris[i] != NULL; i++)
    {
      gchar *filename = g_filename_from_uri (uris[i],
                                             NULL,
                                             NULL);

      t->OpenContest (filename);
      g_free (filename);
    }
    g_strfreev (uris);
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_new_version_menuitem_activate (GtkMenuItem *menuitem,
                                                                  Object       owner)
{
#ifdef WINDOWS_TEMPORARY_PATCH
  ShellExecuteA (NULL,
                 "open",
                 "http://betton.escrime.free.fr/index.php/bellepoule-telechargement",
                 NULL,
                 NULL,
                 SW_SHOWNORMAL);
#else
  gtk_show_uri (NULL,
                "http://betton.escrime.free.fr/index.php/bellepoule-telechargement",
                GDK_CURRENT_TIME,
                NULL);
#endif
}

