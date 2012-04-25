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
#include <gdk/gdkkeysyms.h>
#include <glib/gstdio.h>
#include <curl/curl.h>
#include <tinyxml2.h>

#ifdef WINDOWS_TEMPORARY_PATCH
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

#include "version.h"
#include "contest.hpp"
#if FAKE_XML
#include "fake_xml.h"
#endif

#include "tournament.hpp"

typedef enum
{
  BROADCASTED_ID,
  BROADCASTED_Name,
  BROADCASTED_Weapon,
  BROADCASTED_Gender,
  BROADCASTED_Category
} BroadcastedColumn;

// --------------------------------------------------------------------------------
Tournament::Tournament (gchar *filename)
  : Module ("tournament.glade")
{
  _contest_list = NULL;
  _referee_list = NULL;
  _referee_ref  = 1;
  _nb_matchs    = 0;

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

    OpenUriContest (utf8_name);
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

  {
    GtkWidget *http_entry = _glade->GetWidget ("http_entry");

    gtk_widget_add_events (http_entry,
                           GDK_KEY_PRESS_MASK);
  }

  {
    _competitions_downloader = NULL;

    _version_downloader = new Downloader (OnLatestVersionReceived,
                                          this);
    _version_downloader->Start ("http://betton.escrime.free.fr/documents/BellePoule/latest.html");

    _http_server = new HttpServer (this,
                                   OnGetHttpResponse);
  }

#if FAKE_XML
  gtk_widget_hide (_glade->GetWidget ("logo"));
#endif
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

  {
    GSList *current = _referee_list;

    while (current)
    {
      Player *referee = (Player *) current->data;

      referee->Release ();
      current = g_slist_next (current);
    }
    g_slist_free (_referee_list);
  }

  g_key_file_free (_config_file);

  Object::TryToRelease (_version_downloader);
  Object::TryToRelease (_competitions_downloader);
  _http_server->Release ();
  curl_global_cleanup ();
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
gchar *Tournament::GetHttpResponse (const gchar *url)
{
  gchar *result = NULL;

  if (strstr (url, "/bellepoule/tournament/competition/"))
  {
    gchar *id = strrchr (url, '/');

    if (id && id[1])
    {
      GSList *current = _contest_list;

      id++;
      while (current)
      {
        Contest *contest = (Contest *) current->data;

        if (strcmp (contest->GetId (), id) == 0)
        {
          gsize length;

          g_file_get_contents (contest->GetFilename (),
                               &result,
                               &length,
                               NULL);
          break;
        }
        current = g_slist_next (current);
      }
    }
  }
  else if (strstr (url, "/bellepoule/tournament"))
  {
    GSList  *current  = _contest_list;
    GString *response = g_string_new ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");

    response = g_string_append (response,
                                "<Competitions>\n");
    while (current)
    {
      Contest *contest = (Contest *) current->data;

      response = g_string_append (response,
                                  "<Competition ");

      response = g_string_append (response, "ID=\"");
      response = g_string_append (response, contest->GetId ());
      response = g_string_append (response, "\" ");

      response = g_string_append (response, "Name=\"");
      response = g_string_append (response, contest->GetName ());
      response = g_string_append (response, "\" ");

      response = g_string_append (response, "Weapon=\"");
      response = g_string_append (response, contest->GetWeapon ());
      response = g_string_append (response, "\" ");

      response = g_string_append (response, "Gender=\"");
      response = g_string_append (response, contest->GetGender ());
      response = g_string_append (response, "\" ");

      response = g_string_append (response, "Category=\"");
      response = g_string_append (response, contest->GetCategory ());
      response = g_string_append (response,
                                  "\"/>\n");

      current = g_slist_next (current);
    }
    response = g_string_append (response,
                                "</Competitions>");

    result = response->str;
    g_string_free (response,
                   FALSE);
  }
  else
  {
    result = g_strdup ("");
  }

  return result;
}

// --------------------------------------------------------------------------------
gchar *Tournament::OnGetHttpResponse (Object      *client,
                                      const gchar *url)
{
  Tournament *tournament = dynamic_cast <Tournament *> (client);

  return tournament->GetHttpResponse (url);
}

// --------------------------------------------------------------------------------
Player *Tournament::Share (Player  *referee,
                           Contest *from)
{
  Player *original = NULL;

  {
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
        referee->SetRef (original->GetRef ());
        break;
      }

      current = g_slist_next (current);
    }

    g_slist_free (attr_list);
  }

  if (original == NULL)
  {
    {
      _referee_list = g_slist_prepend (_referee_list,
                                       referee);
      referee->Retain ();
      referee->SetRef (_referee_ref++);
    }

    {
      GSList *current = _contest_list;

      while (current)
      {
        Contest *contest = (Contest *) current->data;

        if (   (contest != from)
            && (contest->GetWeaponCode () == referee->GetWeaponCode ()))
        {
          contest->AddReferee (referee);
        }

        current = g_slist_next (current);
      }
    }
  }

  return original;
}

// --------------------------------------------------------------------------------
void Tournament::RefreshMatchRate (gint delta)
{
  _nb_matchs += delta;

  {
    GSList *current = _referee_list;

    while (current)
    {
      Player *referee = (Player *) current->data;

      RefreshMatchRate (referee);

      current = g_slist_next (current);
    }
  }
}

// --------------------------------------------------------------------------------
void Tournament::RefreshMatchRate (Player *referee)
{
  Player::AttributeId attr_id ("participation_rate");

  if (_nb_matchs)
  {
    referee->SetAttributeValue (&attr_id,
                                referee->GetNbMatchs () * 100 / _nb_matchs);
  }
  else
  {
    referee->SetAttributeValue (&attr_id,
                                (guint) 0);
  }
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

    _contest_list = g_slist_prepend (_contest_list,
                                     contest);
    contest->ImportReferees (_referee_list);

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
    contest->UnPlug ();

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
  GSList *current = _contest_list;

  while (current)
  {
    Contest *contest = (Contest *) current->data;

    if (strcmp (filename, contest->GetFilename ()) == 0)
    {
      return contest;
    }

    current = g_slist_next (current);
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void Tournament::OnNew ()
{
  Contest *contest = Contest::Create ();

  Plug (contest,
        NULL,
        NULL);
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

    OpenUriContest (filename);
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
    Contest *contest = (Contest *) current->data;

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
void Tournament::OpenUriContest (const gchar *uri)
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
      Contest *contest = new Contest ();

      Plug (contest,
            NULL,
            NULL);
      contest->LoadUri (uri);
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
void Tournament::OpenMemoryContest (const gchar *memory)
{
  if (memory)
  {
    SetCursor (GDK_WATCH);

    {
      Contest *contest = new Contest ();

      Plug (contest,
            NULL,
            NULL);
      contest->LoadMemory (memory);
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
  if (_program_path)
  {
    gchar *example_dirname = g_build_filename (_program_path, "Exemples", NULL);

    OnOpen (example_dirname);
    g_free (example_dirname);
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
      OpenUriContest (gtk_recent_info_get_uri_display (info));

      gtk_recent_info_unref (info);
    }
  }

  gtk_widget_destroy (dialog);
}

// --------------------------------------------------------------------------------
void Tournament::OnWifi ()
{
  GtkWidget *w = _glade->GetWidget ("broadcasted_dialog");

  gtk_dialog_run (GTK_DIALOG (w));
  gtk_widget_hide (w);
}

// --------------------------------------------------------------------------------
void Tournament::GetBroadcastedCompetitions ()
{
  GtkWidget *entry   = _glade->GetWidget ("http_entry");
  gchar     *address = g_strdup_printf ("http://%s:8080/bellepoule/tournament",
                                        gtk_entry_get_text (GTK_ENTRY (entry)));

  if (_competitions_downloader == NULL)
  {
    _competitions_downloader = new Downloader (OnCompetitionListReceived,
                                               this);
  }

  _competitions_downloader->Start (address);

  gtk_list_store_clear (GTK_LIST_STORE (_glade->GetWidget ("broadcasted_liststore")));

  gtk_entry_set_icon_from_stock (GTK_ENTRY (_glade->GetWidget ("http_entry")),
                                 GTK_ENTRY_ICON_SECONDARY,
                                 GTK_STOCK_EXECUTE);
}

// --------------------------------------------------------------------------------
void Tournament::StopCompetitionMonitoring ()
{
  if (_competitions_downloader)
  {
    _competitions_downloader->Release ();
    _competitions_downloader = NULL;
  }

  gtk_list_store_clear (GTK_LIST_STORE (_glade->GetWidget ("broadcasted_liststore")));

  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (_glade->GetWidget ("http_entry")),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     "network-offline");
}

// --------------------------------------------------------------------------------
gboolean Tournament::OnCompetitionListReceived (Downloader::CallbackData *cbk_data)
{
  gboolean    error = TRUE;
  Tournament *tournament = (Tournament *) cbk_data->_user_data;
  gchar      *data       = tournament->_competitions_downloader->GetData ();

#if FAKE_XML
  data = fake_competion_list;
#endif
  if (data)
  {
    tinyxml2::XMLDocument *doc = new tinyxml2::XMLDocument ();

    if (doc->Parse (data) == tinyxml2::XML_NO_ERROR)
    {
      GtkListStore         *model = GTK_LIST_STORE (tournament->_glade->GetWidget ("broadcasted_liststore"));
      GtkTreeIter           iter;
      tinyxml2::XMLHandle  *hdl   = new tinyxml2::XMLHandle (doc);
      tinyxml2::XMLHandle   first = hdl->FirstChildElement().FirstChildElement();
      tinyxml2::XMLElement *elem  = first.ToElement ();

      while (elem)
      {
        gtk_list_store_append (model, &iter);
        gtk_list_store_set (model, &iter,
                            BROADCASTED_ID,       elem->Attribute ("ID"),
                            BROADCASTED_Name,     elem->Attribute ("Name"),
                            BROADCASTED_Weapon,   elem->Attribute ("Weapon"),
                            BROADCASTED_Gender,   elem->Attribute ("Gender"),
                            BROADCASTED_Category, elem->Attribute ("Category"),
                            -1);

        elem = elem->NextSiblingElement ();
      }

      free (hdl);
      error = FALSE;
    }
    free (doc);
  }

  if (error)
  {
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (tournament->_glade->GetWidget ("http_entry")),
                                       GTK_ENTRY_ICON_SECONDARY,
                                       "network-offline");
  }
  else
  {
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (tournament->_glade->GetWidget ("http_entry")),
                                       GTK_ENTRY_ICON_SECONDARY,
                                       "network-transmit-receive");
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean Tournament::OnCompetitionReceived (Downloader::CallbackData *cbk_data)
{
  Tournament *tournament = (Tournament *) cbk_data->_user_data;
  gchar      *data       = tournament->_competitions_downloader->GetData ();

#if FAKE_XML
  data = fake_competion;
#endif
  if (data)
  {
    tournament->OpenMemoryContest (data);
  }

  cbk_data->_downloader->Release ();

  return FALSE;
}

// --------------------------------------------------------------------------------
void Tournament::OnBroadcastedActivated (GtkTreePath *path)
{
  GtkTreeModel *model = GTK_TREE_MODEL (_glade->GetWidget ("broadcasted_liststore"));
  GtkTreeIter   iter;
  gchar        *id;

  gtk_tree_model_get_iter (model,
                           &iter,
                           path);

  gtk_tree_model_get (model, &iter,
                      BROADCASTED_ID, &id,
                      -1);

  {
    GtkWidget *entry   = _glade->GetWidget ("http_entry");
    gchar     *address = g_strdup_printf ("http://%s:8080/bellepoule/tournament",
                                          gtk_entry_get_text (GTK_ENTRY (entry)));

    Downloader *downloader = new Downloader (OnCompetitionReceived,
                                             this);

    downloader->Start (address);
  }
}

// --------------------------------------------------------------------------------
void Tournament::OnBackupfileLocation ()
{
  GtkWidget *chooser = GTK_WIDGET (gtk_file_chooser_dialog_new (gettext ("Choose a backup files location..."),
                                                                NULL,
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
gboolean Tournament::OnLatestVersionReceived (Downloader::CallbackData *cbk_data)
{
  Tournament *tournament = (Tournament *) cbk_data->_user_data;
  gchar      *data       = tournament->_version_downloader->GetData ();

  if (data)
  {
    GKeyFile *version_file = g_key_file_new ();

    if (g_key_file_load_from_data (version_file,
                                   data,
                                   strlen (data) + 1,
                                   G_KEY_FILE_NONE,
                                   NULL))
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
  }

  tournament->_version_downloader->Release ();
  tournament->_version_downloader = NULL;

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
extern "C" G_MODULE_EXPORT void on_wifi_menuitem_activate (GtkWidget *w,
                                                             Object    *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnWifi ();
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

      t->OpenUriContest (filename);
      g_free (filename);
    }
    g_strfreev (uris);
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_new_version_menuitem_activate (GtkMenuItem *menuitem,
                                                                  Object      *owner)
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

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT gboolean on_http_entry_key_press_event (GtkWidget *widget,
                                                                   GdkEvent  *event,
                                                                   Object    *owner)
{
  if (event->key.keyval == GDK_Return)
  {
    Tournament *t = dynamic_cast <Tournament *> (owner);

    t->GetBroadcastedCompetitions ();
    return TRUE;
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_broadcasted_treeview_row_activated (GtkTreeView       *tree_view,
                                                                       GtkTreePath       *path,
                                                                       GtkTreeViewColumn *column,
                                                                       Object            *owner)
{
  Tournament  *t = dynamic_cast <Tournament *> (owner);

  t->OnBroadcastedActivated (path);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_http_entry_icon_release (GtkEntry             *entry,
                                                            GtkEntryIconPosition  icon_pos,
                                                            GdkEvent             *event,
                                                            Object               *owner)
{
  Tournament  *t         = dynamic_cast <Tournament *> (owner);
  const gchar *icon_name = gtk_entry_get_icon_name (entry,
                                                    GTK_ENTRY_ICON_SECONDARY);

  if (strcmp (icon_name, "network-offline") == 0)
  {
    t->GetBroadcastedCompetitions ();
  }
  else if (strcmp (icon_name, "network-transmit-receive") == 0)
  {
    t->StopCompetitionMonitoring ();
  }
}
