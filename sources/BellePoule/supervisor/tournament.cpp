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

#include <unistd.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <libxml/xpath.h>

#ifdef WINDOWS_TEMPORARY_PATCH
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

#include "util/global.hpp"
#include "util/canvas.hpp"
#include "util/attribute.hpp"
#include "util/flash_code.hpp"
#include "util/user_config.hpp"
#include "util/glade.hpp"
#include "util/player.hpp"
#include "network/ring.hpp"
#include "network/web_server.hpp"
#include "network/message.hpp"
#include "application/version.h"
#include "application/weapon.hpp"
#include "actors/form.hpp"
#include "network/http_server.hpp"
#include "twitter/twitter.hpp"
#include "facebook/facebook.hpp"
#include "contest.hpp"
#include "publication.hpp"
#include "match.hpp"
#include "askfred/reader.hpp"

#include "tournament.hpp"

// --------------------------------------------------------------------------------
Tournament::Tournament ()
  : Object ("Tournament"),
    Module ("tournament.glade")
{
  _contest_list = NULL;
  _advertisers  = NULL;

  {
    Net::Advertiser *twitter = new Net::Twitter ();

    twitter->Plug (GTK_TABLE (_glade->GetWidget ("advertiser_table")));
    _advertisers = g_list_append (_advertisers,
                                  twitter);
  }

  {
    Net::Advertiser *facebook = new Net::Facebook ();

    facebook->Plug (GTK_TABLE (_glade->GetWidget ("advertiser_table")));
    _advertisers = g_list_append (_advertisers,
                                  facebook);
  }
}

// --------------------------------------------------------------------------------
void Tournament::Init ()
{
  if (Global::_user_config->IsEmpty ())
  {
    g_key_file_set_string (Global::_user_config->_key_file,
                           "Competiton",
                           "default_dir_name",
                           g_get_user_special_dir (G_USER_DIRECTORY_DOCUMENTS));

    g_key_file_set_string (Global::_user_config->_key_file,
                           "Checkin",
                           "default_import_dir_name",
                           g_get_user_special_dir (G_USER_DIRECTORY_DOCUMENTS));
  }
}

// --------------------------------------------------------------------------------
Tournament::~Tournament ()
{
  {
    GList *death_row = g_list_copy (_contest_list);
    GList *current   = death_row;

    while (current)
    {
      Contest *contest = (Contest *) current->data;

      contest->Release ();
      current = g_list_next (current);
    }

    g_list_free (death_row);
  }

  _web_server->Release  ();
  _publication->Release ();
  Contest::Cleanup ();

  FreeFullGList(Net::Advertiser, _advertisers);
}

// --------------------------------------------------------------------------------
void Tournament::Start (gchar *filename)
{
  _publication = new Publication (_glade);

  gtk_widget_hide (_glade->GetWidget ("notebook"));

  // URL QrCode
  {
    gchar *html_url;

    html_url = g_strdup_printf ("http://%s/index.php", Net::Ring::_broker->GetIpV4Address ());

    SetFlashRef (html_url);

    g_free (html_url);
  }

  if (filename)
  {
    gchar *utf8_name = g_locale_to_utf8 (filename,
                                         -1,
                                         NULL,
                                         NULL,
                                         NULL);

    OpenUriContest (utf8_name);
    g_free (utf8_name);

    g_free (filename);
  }

  {
    gchar *last_backup = g_key_file_get_string (Global::_user_config->_key_file,
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

  _web_server = new Net::WebServer ((Net::WebServer::StateFunc) OnWebServerState,
                                    this);
}

// --------------------------------------------------------------------------------
gchar *Tournament::GetSecretKey (const gchar *authentication_scheme)
{
  gchar *key = NULL;

  if (authentication_scheme)
  {
    if (   (g_strcmp0 (authentication_scheme, "/ring") == 0)
        || (g_strcmp0 (authentication_scheme, "/MarshallerCredentials/0") == 0))
    {
      return Net::Ring::_broker->GetCryptorKey ();
    }
    else
    {
      gchar **tokens = g_strsplit_set (authentication_scheme,
                                       "/",
                                       0);

      if (tokens)
      {
        if (tokens[0] && tokens[1] && tokens[2])
        {
          guint ref = atoi (tokens[2]);

          if (ref == 0)
          {
            g_warning ("Tournament::GetSecretKey");
            //WifiCode *wifi_code = _publication->GetAdminCode ();

            //key = wifi_code->GetKey ();
          }
          else
          {
            GList *current = _contest_list;

            while (current)
            {
              Contest *contest = (Contest *) current->data;
              Player  *referee = contest->GetRefereeFromRef (ref);

              if (referee)
              {
                Player::AttributeId  attr_id ("password");
                Attribute           *attr   = referee->GetAttribute (&attr_id);

                if (attr)
                {
                  key = g_strdup (attr->GetStrValue ());
                  break;
                }
              }
              current = g_list_next (current);
            }
          }
          g_strfreev (tokens);
        }
      }
    }
  }

  return key;
}

// --------------------------------------------------------------------------------
Player *Tournament::UpdateConnectionStatus (GList       *player_list,
                                            guint        ref,
                                            const gchar *ip_address,
                                            const gchar *status)
{
  GList *current = player_list;

  while (current)
  {
    Player *current_player = (Player *) current->data;

    if (current_player->GetRef () == ref)
    {
      Player::AttributeId connection_attr_id ("connection");
      Player::AttributeId ip_attr_id         ("IP");

      current_player->SetAttributeValue (&connection_attr_id,
                                         status);
      if (ip_address)
      {
        current_player->SetAttributeValue (&ip_attr_id,
                                           ip_address);
      }
      return current_player;
    }
    current = g_list_next (current);
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void Tournament::OnShowAccessCode (gboolean with_steps)
{
  {
    GtkWidget *w;

    w = _glade->GetWidget ("step1");
    gtk_widget_set_visible (w, with_steps);

    w = _glade->GetWidget ("step2");
    gtk_widget_set_visible (w, with_steps);
  }

  {
    GtkDialog *dialog = GTK_DIALOG (_glade->GetWidget ("pin_code_dialog"));

    {
      FlashCode *flash_code = Net::Ring::_broker->GetFlashCode ();
      GdkPixbuf *pixbuf     = flash_code->GetPixbuf ();
      GtkImage  *image      = GTK_IMAGE (_glade->GetWidget ("qrcode_image"));

      gtk_image_set_from_pixbuf (image,
                                 pixbuf);
      g_object_unref (pixbuf);
    }

    RunDialog (dialog);
    gtk_widget_hide (GTK_WIDGET (dialog));
  }
}

// --------------------------------------------------------------------------------
void Tournament::OnHanshakeResult (Net::Ring::HandshakeResult result)
{
  if (result == Net::Ring::AUTHENTICATION_FAILED)
  {
    OnShowAccessCode (TRUE);
  }
}

// --------------------------------------------------------------------------------
gboolean Tournament::OnHttpPost (Net::Message *message)
{
  gboolean result = FALSE;

  if (message->Is ("RegisteredReferee"))
  {
    GList *current = _contest_list;

    while (current)
    {
      Contest *contest = (Contest *) current->data;

      contest->ManageReferee (message);

      current = g_list_next (current);
    }

    result = TRUE;
  }
  else if (message->Is ("Roadmap"))
  {
    Contest *contest = GetContest (message->GetInteger ("competition"));

    if (contest)
    {
      result = contest->OnMessage (message);
    }
  }
  else if (message->Is ("EndOfBurst"))
  {
    Contest *contest = GetContest (message->GetInteger ("competition"));

    if (contest)
    {
      result = contest->OnMessage (message);
    }
  }
  else if (message->Is ("ScoreSheetCall"))
  {
    Contest *contest = GetContest (message->GetInteger ("competition"));

    if (contest)
    {
      result = contest->OnMessage (message);
    }
  }
  else if (message->Is ("Score"))
  {
    Contest *contest = GetContest (message->GetInteger ("competition"));

    if (contest)
    {
      result = contest->OnMessage (message);
    }
  }
  else if (message->Is ("Handshake"))
  {
    Net::Ring::_broker->OnHandshake (message,
                                     this);
  }
  else if (message->Is ("ClockOffset"))
  {
    Match::OnClockOffset (message);
  }
  else if (message->Is ("MarshallerCredentials"))
  {
    GtkDialog *dialog = GTK_DIALOG (_glade->GetWidget ("pin_code_dialog"));

    gtk_dialog_response (dialog,
                         GTK_RESPONSE_OK);

    {
      gchar *passphrase = message->GetString ("marshaller_key");

      Net::Ring::_broker->ChangePassphrase (passphrase);
      g_free (passphrase);
    }
  }
  else
  {
    gchar *data = message->GetString ("content");

    if (data)
    {
      gchar **lines = g_strsplit_set (data,
                                      "\n",
                                      0);
      if (lines[0])
      {
        const gchar *body = data;

        body = strstr (body, "\n"); if (body) body++;

        // Request type
        if (lines[1])
        {
          gchar **tokens = g_strsplit_set (lines[1],
                                           "/",
                                           0);

          body = strstr (body, "\n"); if (body) body++;
          if (tokens && tokens[0] && tokens[1])
          {
            // Competition data
            if (g_strcmp0 (tokens[1], "ScoreSheet") == 0) // ex: /ScoreSheet/Competition/61/2/1
            {
              if (tokens[2] && (g_strcmp0 (tokens[2], "Competition") == 0))
              {
                guint    competition_id = g_ascii_strtoull (tokens[3], NULL, 0);
                Contest *contest        = GetContest (competition_id);

                if (contest)
                {
                  GtkNotebook *nb  = GTK_NOTEBOOK (_glade->GetWidget ("notebook"));
                  gint        page = gtk_notebook_page_num (nb,
                                                            contest->GetRootWidget ());

                  g_object_set (G_OBJECT (nb),
                                "page", page,
                                NULL);

                  result = contest->OnHttpPost (tokens[1],
                                                (const gchar**) &tokens[4],
                                                body);
                }
              }
            }
          }
          g_strfreev (tokens);
        }
      }
      g_strfreev (lines);
      g_free (data);
    }
  }

  return result;
}

// --------------------------------------------------------------------------------
void Tournament::Manage (Contest *contest)
{
  if (contest)
  {
    GtkWidget *nb = _glade->GetWidget ("notebook");

    contest->AttachTo (GTK_NOTEBOOK (nb));

    _contest_list = g_list_prepend (_contest_list,
                                    contest);

    if (g_list_length (_contest_list) == 1)
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
    _contest_list = g_list_remove (_contest_list,
                                   contest);
    if (g_list_length (_contest_list) == 0)
    {
      gtk_widget_show (_glade->GetWidget ("logo"));
      gtk_widget_hide (_glade->GetWidget ("notebook"));
    }
  }
}

// --------------------------------------------------------------------------------
Contest *Tournament::GetContest (guint netid)
{
  GList *current = _contest_list;

  while (current)
  {
    Contest *contest = (Contest *) current->data;

    if (netid == contest->GetNetID ())
    {
      return contest;
    }

    current = g_list_next (current);
  }

  return NULL;
}

// --------------------------------------------------------------------------------
Contest *Tournament::GetContest (const gchar *filename)
{
  GList *current = _contest_list;

  while (current)
  {
    Contest *contest = (Contest *) current->data;

    if (g_strcmp0 (filename, contest->GetFilename ()) == 0)
    {
      return contest;
    }

    current = g_list_next (current);
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void Tournament::OnNew ()
{
  Contest *contest = new Contest (_advertisers);

  Manage (contest);
  contest->AskForSettings ();

  Plug (contest,
        NULL);
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
                              gettext ("Competition files (.cotcot / .xml / .frd)"));
    gtk_file_filter_add_pattern (filter,
                                 "*.cotcot");
    gtk_file_filter_add_pattern (filter,
                                 "*.xml");
    gtk_file_filter_add_pattern (filter,
                                 "*.XML");
    gtk_file_filter_add_pattern (filter,
                                 "*.frd");
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
    gchar *last_dirname = g_key_file_get_string (Global::_user_config->_key_file,
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

  if (RunDialog (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
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
  GList *current = _contest_list;

  while (current)
  {
    Contest *contest = (Contest *) current->data;

    contest->Save ();

    current = g_list_next (current);
  }
}

// --------------------------------------------------------------------------------
void Tournament::OnOpenTemplate ()
{
  GString *contents = g_string_new ("");
  gchar   *filename = g_build_filename (g_get_user_special_dir (G_USER_DIRECTORY_TEMPLATES),
                                        "fencer_file_template.csv",
                                        NULL);

  {
    GList *current_desc = AttributeDesc::GetList ();

    while (current_desc)
    {
      AttributeDesc *desc = (AttributeDesc *) current_desc->data;

      if (desc->MatchCriteria ("CSV ready"))
      {
        GError      *error         = NULL;
        const gchar *to_codeset;
        gchar       *locale_string;

        g_get_charset (&to_codeset);
        locale_string = g_convert_with_fallback (desc->_user_name,
                                                 -1,
                                                 to_codeset,
                                                 "UTF-8",
                                                 desc->_code_name,
                                                 NULL,
                                                 NULL,
                                                 &error);
        if (error)
        {
          g_warning ("g_convert_with_fallback: %s", error->message);
          g_error_free (error);
        }

        contents = g_string_append (contents,
                                    locale_string);
        contents = g_string_append_c (contents,
                                      ';');

        g_free (locale_string);
      }

      current_desc = g_list_next (current_desc);
    }
  }

  if (g_file_set_contents (filename,
                           contents->str,
                           -1,
                           NULL))
  {
    gchar *uri;

#ifdef WINDOWS_TEMPORARY_PATCH
    uri = g_locale_from_utf8 (filename,
                              -1,
                              NULL,
                              NULL,
                              NULL);

    ShellExecute (NULL,
                  "open",
                  uri,
                  NULL,
                  NULL,
                  SW_SHOWNORMAL);
#else
    uri = g_filename_to_uri (filename,
                             NULL,
                             NULL);
    gtk_show_uri (NULL,
                  uri,
                  GDK_CURRENT_TIME,
                  NULL);
#endif

    g_free (uri);
  }

  g_string_free (contents,
                 TRUE);
  g_free (filename);
}

// --------------------------------------------------------------------------------
void Tournament::OpenUriContest (const gchar *uri)
{
  if (uri)
  {
    SetCursor (GDK_WATCH);

    {
      gchar *dirname = g_path_get_dirname (uri);

      g_key_file_set_string (Global::_user_config->_key_file,
                             "Competiton",
                             "default_dir_name",
                             dirname);
      g_free (dirname);
    }

    if (g_str_has_suffix (uri,
                          ".frd"))
    {
      AskFred::Reader::Parser *askfred = new AskFred::Reader::Parser (uri);
      GList                   *events  = askfred->GetEvents ();
      gchar                   *dirname = g_path_get_dirname (uri);

      while (events)
      {
        AskFred::Reader::Event *event   = (AskFred::Reader::Event *) events->data;
        Contest                *contest = new Contest (_advertisers);

        Plug (contest,
              NULL);

        contest->LoadAskFred (event,
                              dirname);
        Manage (contest);

        events = g_list_next (events);
      }

      g_free (dirname);
      askfred->Release ();

      {
        GtkWidget *dialog;

        dialog = gtk_message_dialog_new (NULL,
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_INFO,
                                         GTK_BUTTONS_OK,
                                         "AskFred import done");

        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                                  "Adjust the imported data if needed and \ndon't forget to save all the created events!");

        RunDialog (GTK_DIALOG(dialog));
        gtk_widget_destroy (dialog);
      }
    }
    else
    {
      static const gchar *contest_suffix_table[] = {".cotcot", ".COTCOT", ".xml", ".XML", NULL};
      static const gchar *people_suffix_table[]  = {".fff", ".FFF", ".csv", ".CSV", ".txt", ".TXT", NULL};
      Contest            *contest = NULL;

      for (guint i = 0; contest_suffix_table[i] != NULL; i++)
      {
        if (g_str_has_suffix (uri,
                              contest_suffix_table[i]))
        {
          contest = new Contest (_advertisers);

          contest->LoadXml (uri);

          Manage (contest);
          Plug (contest,
                NULL);

          break;
        }
      }

      for (guint i = 0; people_suffix_table[i] != NULL; i++)
      {
        if (g_str_has_suffix (uri,
                              people_suffix_table[i]))
        {
          contest = new Contest (_advertisers);

          contest->LoadFencerFile (uri);

          Manage (contest);
          contest->AskForSettings ();
          Plug (contest,
                NULL);

          break;
        }
      }
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
  if (Global::_share_dir)
  {
    gchar *example_dirname = g_build_filename (Global::_share_dir, "Exemples", NULL);

    OnOpen (example_dirname);
    g_free (example_dirname);
  }
}

// --------------------------------------------------------------------------------
gboolean Tournament::RecentFileExists (const GtkRecentFilterInfo *filter_info,
                                       Tournament                *tournament)
{
  gboolean exists = FALSE;

  if (filter_info->contains & GTK_RECENT_FILTER_URI)
  {
    gchar *filename = g_filename_from_uri (filter_info->uri,
                                           NULL,
                                           NULL);

    if (filename && g_str_has_suffix (filename, ".cotcot"))
    {
      if (g_file_test (filename, G_FILE_TEST_EXISTS))
      {
        exists = TRUE;
      }
    }
    g_free (filename);
  }

  return exists;
}

// --------------------------------------------------------------------------------
void Tournament::OnRecent ()
{
  GtkWidget *dialog = gtk_recent_chooser_dialog_new (gettext ("Recently opened files"),
                                                     NULL,
                                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                     GTK_STOCK_OPEN,   GTK_RESPONSE_ACCEPT,
                                                     NULL);

  gtk_recent_chooser_set_show_tips (GTK_RECENT_CHOOSER (dialog),
                                    TRUE);

  // gtk_recent_chooser_set_show_not_found (...) doesn't work well on windows.
  // with utf8 encoded filename. For this reason we implement our own filter
  {
    GtkRecentFilter *filter = gtk_recent_filter_new ();

    gtk_recent_filter_add_custom (filter,
                                  (GtkRecentFilterFlags) (GTK_RECENT_FILTER_URI|GTK_RECENT_FILTER_DISPLAY_NAME),
                                  (GtkRecentFilterFunc) RecentFileExists,
                                  this,
                                  NULL);

    gtk_recent_filter_set_name (filter,
                                gettext ("BellePoule files"));

    gtk_recent_chooser_add_filter (GTK_RECENT_CHOOSER (dialog),
                                   filter);
  }

  if (RunDialog (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
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
void Tournament::OnMenuDialog (const gchar *dialog)
{
  GtkWidget *w = _glade->GetWidget (dialog);

  RunDialog (GTK_DIALOG (w));
  gtk_widget_hide (w);
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
    gchar *last_location = g_key_file_get_string (Global::_user_config->_key_file,
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

  if (RunDialog (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
  {
    gchar *foldername = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (chooser));

    if (foldername)
    {
      g_key_file_set_string (Global::_user_config->_key_file,
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
void Tournament::OnVideoReleased ()
{
  GtkToggleButton *togglebutton = GTK_TOGGLE_BUTTON (_glade->GetWidget ("video_off"));

  if (gtk_toggle_button_get_active (togglebutton))
  {
    _web_server->Stop ();
  }
  else
  {
    _web_server->Start ();
  }
}

// --------------------------------------------------------------------------------
Publication *Tournament::GetPublication ()
{
  return _publication;
}

// --------------------------------------------------------------------------------
void Tournament::OnWebServerState (gboolean  in_progress,
                                   gboolean  on,
                                   Object   *owner)
{
  Tournament      *t          = dynamic_cast <Tournament *> (owner);
  GtkWidget       *hbox       = t->_glade->GetWidget ("video_toggle_hbox");
  GtkWidget       *spinner    = t->_glade->GetWidget ("video_spinner");
  GtkToggleButton *on_toggle  = GTK_TOGGLE_BUTTON (t->_glade->GetWidget ("video_on"));
  GtkToggleButton *off_toggle = GTK_TOGGLE_BUTTON (t->_glade->GetWidget ("video_off"));

  gtk_toggle_button_set_active (on_toggle,  (on == TRUE));
  gtk_toggle_button_set_active (off_toggle, (on == FALSE));
  if (in_progress == TRUE)
  {
    gtk_widget_set_sensitive (hbox, FALSE);
    gtk_widget_show (spinner);
  }
  else
  {
    gtk_widget_set_sensitive (hbox, TRUE);
    gtk_widget_hide (spinner);
  }
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
extern "C" G_MODULE_EXPORT void on_example_menuitem_activate (GtkWidget *w,
                                                              Object    *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnOpenExample ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_template_imagemenuitem_activate (GtkWidget *w,
                                                                    Object    *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnOpenTemplate ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_recent_menuitem_activate (GtkWidget *w,
                                                             Object    *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnRecent ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_ftp_menuitem_activate (GtkWidget *w,
                                                          Object    *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnMenuDialog ("publication_dialog");
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
extern "C" G_MODULE_EXPORT void on_video_released (GtkWidget *widget,
                                                   Object    *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnVideoReleased ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_access_code_activate (GtkWidget *w,
                                                         Object    *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnShowAccessCode (FALSE);
}

