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
#include <locale.h>

#ifdef WINDOWS_TEMPORARY_PATCH
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

#include "util/global.hpp"
#include "util/canvas.hpp"
#include "network/ring.hpp"
#include "application/version.h"
#include "application/weapon.hpp"
#include "actors/form.hpp"
#include "network/http_server.hpp"
#include "contest.hpp"
#include "ask_fred.hpp"

#include "tournament.hpp"

// --------------------------------------------------------------------------------
Tournament::Tournament ()
  : Object ("Tournament"),
    Module ("tournament.glade")
{
  _contest_list = NULL;
  _referee_list = NULL;
  _referee_ref  = 1;

  _twitter = new Net::Twitter (this);
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
  FreeFullGList (Contest, _contest_list);
  FreeFullGList (Player, _referee_list);

  _web_server->Release  ();
  _ecosystem->Release   ();
  Contest::Cleanup ();

  _twitter->Release ();
}

// --------------------------------------------------------------------------------
void Tournament::Start (gchar *filename)
{
  _ecosystem = new EcoSystem (_glade);

  gtk_widget_hide (_glade->GetWidget ("notebook"));

  // URL QrCode
  {
    gchar *html_url;

    html_url = g_strdup_printf ("http://%s/index.php", Net::Ring::GetIpV4Address ());

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
  WifiCode *wifi_code = NULL;

  if (authentication_scheme)
  {
    gchar **tokens = g_strsplit_set (authentication_scheme,
                                     "/",
                                     0);

    if (tokens && tokens[0] && tokens[1] && tokens[2])
    {
      if (g_strcmp0 (tokens[1], "ScoreSheet") == 0)
      {
        wifi_code = _ecosystem->GetAdminCode ();
      }
      else if (   (g_strcmp0 (tokens[1], "HandShake") == 0)
               || (g_strcmp0 (tokens[1], "Score")     == 0))
      {
        GList *current = _referee_list;
        guint  ref     = atoi (tokens[2]);

        if (ref == 0)
        {
          wifi_code = _ecosystem->GetAdminCode ();
        }
        else
        {
          while (current)
          {
            Player *referee = (Player *) current->data;

            if (referee->GetRef () == ref)
            {
              wifi_code = (WifiCode *) referee->GetFlashCode ();
              break;
            }

            current = g_list_next (current);
          }
        }
      }
      g_strfreev (tokens);
    }
  }

  if (wifi_code)
  {
    return wifi_code->GetKey ();
  }

  return NULL;
}

// --------------------------------------------------------------------------------
guint Tournament::PreparePrint (GtkPrintOperation *operation,
                                GtkPrintContext   *context)
{
  guint list_length = g_list_length (_referee_list);
  guint n;

  if (_print_meal_tickets)
  {
    n = list_length / (NB_TICKET_Y_PER_SHEET * NB_TICKET_X_PER_SHEET);

    if (list_length % (NB_TICKET_Y_PER_SHEET * NB_TICKET_X_PER_SHEET))
    {
      n++;
    }
  }
  else
  {
    n = list_length / NB_REFEREE_PER_SHEET;

    if (list_length % NB_REFEREE_PER_SHEET)
    {
      n++;
    }
  }

  return n;
}

// --------------------------------------------------------------------------------
void Tournament::DrawPage (GtkPrintOperation *operation,
                           GtkPrintContext   *context,
                           gint               page_nr)
{
  gdouble        paper_w  = gtk_print_context_get_width  (context);
  gdouble        paper_h  = gtk_print_context_get_height (context);
  cairo_t       *cr       = gtk_print_context_get_cairo_context (context);
  GooCanvas     *canvas   = Canvas::CreatePrinterCanvas (context);
  GooCanvasItem *main_table;
  GooCanvasItem *item;
  GList         *current;

  cairo_save (cr);

  if (_print_meal_tickets)
  {
    gdouble spacing  = 2.0;
    gdouble ticket_w = (100.0/NB_TICKET_X_PER_SHEET) - (NB_TICKET_X_PER_SHEET-1) *spacing;
    gdouble ticket_h = (100.0/NB_TICKET_Y_PER_SHEET) *paper_h/paper_w - (NB_TICKET_Y_PER_SHEET-1) *spacing;
    gdouble border_w = 0.7;

    main_table = goo_canvas_table_new (goo_canvas_get_root_item (canvas),
                                       "row-spacing",         2.0,
                                       "column-spacing",      2.0,
                                       "homogeneous-columns", TRUE,
                                       "homogeneous-rows",    TRUE,
                                       NULL);

    current = g_list_nth (_referee_list,
                          NB_TICKET_Y_PER_SHEET*NB_TICKET_X_PER_SHEET*page_nr);

    for (guint r = 0; current && (r < NB_TICKET_Y_PER_SHEET); r++)
    {
      for (guint c = 0; current && (c < NB_TICKET_X_PER_SHEET); c++)
      {
        Player        *referee = (Player *) current->data;
        GooCanvasItem *ticket_table = goo_canvas_table_new (main_table,
                                                            "x-border-spacing", 1.0,
                                                            "y-border-spacing", 1.0,
                                                            NULL);
        // Border
        {
          item = goo_canvas_rect_new (main_table,
                                      0.0,
                                      0.0,
                                      ticket_w,
                                      ticket_h,
                                      "stroke-color", "Grey",
                                      "line-width",   border_w,
                                      NULL);
          Canvas::PutInTable (main_table,
                              item,
                              r,
                              c);
        }

        {
          {
            GooCanvasItem *name_table = goo_canvas_table_new (ticket_table, NULL);

            // Name
            {
              gchar               *font   = g_strdup_printf (BP_FONT "Bold %fpx", 2*PRINT_FONT_HEIGHT);
              Player::AttributeId  attr_id ("name");
              Attribute           *attr   = referee->GetAttribute (&attr_id);
              gchar               *string = attr->GetUserImage (AttributeDesc::LONG_TEXT);

              Canvas::NormalyzeDecimalNotation (font);
              item = Canvas::PutTextInTable (name_table,
                                             string,
                                             0,
                                             0);
              g_object_set (G_OBJECT (item),
                            "font",      font,
                            "ellipsize", PANGO_ELLIPSIZE_END,
                            "width",     ticket_w/4.0 - 2.0*border_w,
                            NULL);
              g_free (string);
              g_free (font);
            }

            // First name
            {
              gchar               *font   = g_strdup_printf (BP_FONT "%fpx", 1.5*PRINT_FONT_HEIGHT);
              Player::AttributeId  attr_id ("first_name");
              Attribute           *attr   = referee->GetAttribute (&attr_id);
              gchar               *string = attr->GetUserImage (AttributeDesc::LONG_TEXT);

              Canvas::NormalyzeDecimalNotation (font);
              item = Canvas::PutTextInTable (name_table,
                                             string,
                                             1,
                                             0);
              g_object_set (G_OBJECT (item),
                            "font", font,
                            "ellipsize", PANGO_ELLIPSIZE_END,
                            "width",     ticket_w/4.0 - 2.0*border_w,
                            NULL);
              g_free (string);
              g_free (font);
            }

            Canvas::PutInTable (ticket_table,
                                name_table,
                                0,
                                0);
            Canvas::SetTableItemAttribute (name_table, "x-expand", 1U);
            Canvas::SetTableItemAttribute (name_table, "x-fill",   1U);
          }

          // Food
          {
            GooCanvasItem *food_table = goo_canvas_table_new (ticket_table, NULL);
            gchar         *font   = g_strdup_printf (BP_FONT "%fpx", 1.8*PRINT_FONT_HEIGHT);
            const gchar   *strings[] = {gettext ("Meal"),
                                        gettext ("Drink"),
                                        gettext ("Dessert"),
                                        gettext ("Coffee"), NULL};

            Canvas::NormalyzeDecimalNotation (font);

            g_object_set (G_OBJECT (food_table),
                          "horz-grid-line-width", 0.3,
                          "stroke-color", "White",
                          "fill-color",   "grey",
                          NULL);
            for (guint i = 0; strings[i] != NULL; i++)
            {
              item = Canvas::PutTextInTable (food_table,
                                             gettext (strings[i]),
                                             i,
                                             0);
              g_object_set (G_OBJECT (item),
                            "font",       font,
                            "fill-color", "Black",
                            NULL);
              Canvas::SetTableItemAttribute (item, "x-align", 1.0);
              Canvas::SetTableItemAttribute (item, "x-fill",   0U);
            }

            g_free (font);

            Canvas::PutInTable (ticket_table,
                                food_table,
                                0,
                                1);
            Canvas::SetTableItemAttribute (food_table, "right-padding", 1.0);
          }
        }

        Canvas::PutInTable (main_table,
                            ticket_table,
                            r,
                            c);
        Canvas::SetTableItemAttribute (ticket_table, "x-expand", 1U);
        Canvas::SetTableItemAttribute (ticket_table, "x-fill",   1U);
        current = g_list_next (current);
      }
    }
  }
  else
  {
    gchar         *main_font = g_strdup_printf (BP_FONT "%fpx", 1.0*PRINT_FONT_HEIGHT);
    GooCanvasItem *header;

    Canvas::NormalyzeDecimalNotation (main_font);

    current = g_list_nth (_referee_list,
                          NB_REFEREE_PER_SHEET*page_nr);

    main_table = goo_canvas_table_new (goo_canvas_get_root_item (canvas),
                                       "row-spacing",          4.0,
                                       "y-border-spacing",     2.0,
                                       "column-spacing",       10.0,
                                       "horz-grid-line-width", 0.2,
                                       "homogeneous-rows",     TRUE,
                                       NULL);

    {
      gchar *font = g_strdup_printf (BP_FONT "Bold %fpx", 3.0*PRINT_FONT_HEIGHT);

      Canvas::NormalyzeDecimalNotation (font);
      header = goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                                    gettext ("Payment book"),
                                    50.0,
                                    1.0,
                                    -1.0,
                                    GTK_ANCHOR_CENTER,
                                    "font", font,
                                    NULL);
      g_free (font);
    }

    {
      gchar *font   = g_strdup_printf (BP_FONT "Bold %fpx", 1.0*PRINT_FONT_HEIGHT);
      gchar *string = g_strdup_printf ("%s %d", gettext ("Page"), page_nr+1);

      Canvas::NormalyzeDecimalNotation (font);
      goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                           string,
                           100.0,
                           100.0 * paper_h/paper_w,
                           -1.0,
                           GTK_ANCHOR_SE,
                           "font", font,
                           NULL);
      g_free (string);
      g_free (font);
    }

    for (guint r = 0; current && (r < NB_REFEREE_PER_SHEET); r++)
    {
      Player *referee = (Player *) current->data;
      guint   c       = 0;

      {
        const gchar *column[] = {"name", "first_name", "level", NULL};

        for (guint i = 0; column[i] != NULL; i++)
        {
          Player::AttributeId  attr_id (column[i]);
          Attribute           *attr   = referee->GetAttribute (&attr_id);
          gchar               *string;

          if (attr)
          {
            string = attr->GetUserImage (AttributeDesc::LONG_TEXT);
          }
          else
          {
            string = g_strdup ("-");
          }

          item = Canvas::PutTextInTable (main_table,
                                         string,
                                         r,
                                         c);
          Canvas::SetTableItemAttribute (item, "y-align", 0.5);
          g_object_set (G_OBJECT (item),
                        "font", main_font,
                        NULL);
          g_free (string);
          c++;
        }
      }

      {
        struct lconv *local_conv = localeconv ();
        const gchar  *column[]   = {local_conv->currency_symbol, gettext ("Signature"), NULL};

        for (guint i = 0; column[i] != NULL; i++)
        {
          item = Canvas::PutTextInTable (main_table,
                                         gettext (column[i]),
                                         r,
                                         c);
          Canvas::SetTableItemAttribute (item, "x-align", 0.5);
          Canvas::SetTableItemAttribute (item, "y-align", 0.5);
          g_object_set (G_OBJECT (item),
                        "font", main_font,
                        "fill-color", "grey",
                        NULL);
          c++;
        }
      }

      current = g_list_next (current);
    }

    Canvas::Anchor (main_table,
                    header,
                    NULL,
                    40);
#if 0
    if (_print_scale != 1.0)
    {
      cairo_matrix_t matrix;

      goo_canvas_item_get_transform (goo_canvas_get_root_item (canvas),
                                     &matrix);
      cairo_matrix_scale (&matrix,
                          _print_scale,
                          _print_scale);

      goo_canvas_item_set_transform (goo_canvas_get_root_item (canvas),
                                     &matrix);
    }
#endif
  }

  goo_canvas_render (canvas,
                     cr,
                     NULL,
                     1.0);
  gtk_widget_destroy (GTK_WIDGET (canvas));

  cairo_restore (cr);
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
                                         "OK");
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
gboolean Tournament::OnHttpPost (Net::Message *message)
{
  gboolean  result = FALSE;

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
        // Source
        {
          gchar **tokens = g_strsplit_set (lines[0],
                                           "/",
                                           0);

          if (tokens && tokens[0] && tokens[1] && tokens[2]) // ex: /10.12.74.121/1
          {
            // Status feedback
            UpdateConnectionStatus (_referee_list,
                                    atoi (tokens[2]),
                                    tokens[1],
                                    "OK");
          }
          g_strfreev (tokens);
        }

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
            if (   (g_strcmp0 (tokens[1], "Score") == 0)       // ex: /Score/Competition/61/2/1/1
                || (g_strcmp0 (tokens[1], "ScoreSheet") == 0)) // ex: /ScoreSheet/Competition/61/2/1
            {
              if (tokens[2] && (g_strcmp0 (tokens[2], "Competition") == 0))
              {
                guint    competition_id = g_ascii_strtoull (tokens[3], NULL, 0);
                Contest *contest        = GetContest (competition_id);

                if (contest)
                {
                  if (g_strcmp0 (tokens[1], "ScoreSheet") == 0)
                  {
                    GtkNotebook *nb  = GTK_NOTEBOOK (_glade->GetWidget ("notebook"));
                    gint        page = gtk_notebook_page_num (nb,
                                                              contest->GetRootWidget ());

                    g_object_set (G_OBJECT (nb),
                                  "page", page,
                                  NULL);
                  }

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
  Contest *contest = new Contest ();

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
        gchar *locale_string = g_locale_from_utf8 (desc->_user_name,
                                                   -1,
                                                   NULL,
                                                   NULL,
                                                   NULL);
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
      AskFred::Parser *askfred = new AskFred::Parser (uri);
      GList           *events  = askfred->GetEvents ();
      gchar           *dirname = g_path_get_dirname (uri);

      while (events)
      {
        AskFred::Event *event   = (AskFred::Event *) events->data;
        Contest        *contest = new Contest ();

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
          contest = new Contest ();

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
          contest = new Contest ();

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
void Tournament::PrintMealTickets ()
{
  _print_meal_tickets = TRUE;
  Print (gettext ("Meal tickets"));
}

// --------------------------------------------------------------------------------
void Tournament::PrintPaymentBook ()
{
  _print_meal_tickets = FALSE;
  Print (gettext ("Payment book"));
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
void Tournament::OnTwitterToggled (gboolean on)
{
  if (on)
  {
    GtkWidget *togglebutton = _glade->GetWidget ("twitter_checkbutton");
    GtkWidget *spinner      = _glade->GetWidget ("twitter_spinner");
    GtkEntry  *entry        = GTK_ENTRY (_glade->GetWidget ("twitter_id"));

    gtk_widget_show (spinner);
    gtk_widget_set_sensitive (togglebutton,
                              FALSE);

    gtk_entry_set_text (entry,
                        "");

    _twitter->SwitchOn ();
  }
  else
  {
    _twitter->SwitchOff ();
  }
}

// --------------------------------------------------------------------------------
void Tournament::OnTwitterID (const gchar *id)
{
  GtkWidget *togglebutton = _glade->GetWidget ("twitter_checkbutton");
  GtkWidget *spinner      = _glade->GetWidget ("twitter_spinner");
  GtkEntry  *entry        = GTK_ENTRY (_glade->GetWidget ("twitter_id"));

  gtk_widget_hide (spinner);
  gtk_widget_set_sensitive (togglebutton,
                            TRUE);

  gtk_entry_set_text (entry,
                      id);

  if (g_strstr_len (id,
                    -1,
                    "@") == NULL)
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (togglebutton),
                                  FALSE);
  }

}

// --------------------------------------------------------------------------------
void Tournament::OnResetTwitterAccount ()
{
  GtkEntry        *entry        = GTK_ENTRY         (_glade->GetWidget ("twitter_id"));
  GtkToggleButton *togglebutton = GTK_TOGGLE_BUTTON (_glade->GetWidget ("twitter_checkbutton"));

  gtk_toggle_button_set_active (togglebutton,
                                FALSE);

  gtk_entry_set_text (entry,
                      "");

  _twitter->Reset ();
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
EcoSystem *Tournament::GetEcosystem ()
{
  return _ecosystem;
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
extern "C" G_MODULE_EXPORT void on_save_menuitem_activate (GtkWidget *w,
                                                           Object    *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnSave ();
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
extern "C" G_MODULE_EXPORT void on_network_config_menuitem_activate (GtkWidget *w,
                                                                     Object    *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnMenuDialog ("network_dialog");
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_ftp_menuitem_activate (GtkWidget *w,
                                                          Object    *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnMenuDialog ("publication_dialog");
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_scanner_menuitem_activate (GtkWidget *w,
                                                              Object    *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnMenuDialog ("scanner_dialog");
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
extern "C" G_MODULE_EXPORT void on_twitter_checkbutton_toggled (GtkToggleButton *button,
                                                                Object          *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnTwitterToggled (gtk_toggle_button_get_active (button));
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
extern "C" G_MODULE_EXPORT void on_tickets_menuitem_activate (GtkMenuItem *menuitem,
                                                              Object      *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->PrintMealTickets ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_payment_menuitem_activate (GtkMenuItem *menuitem,
                                                              Object      *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->PrintPaymentBook ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_SmartPoule_button_clicked (GtkButton *widget,
                                                              gpointer   user_data)
{
#ifdef WINDOWS_TEMPORARY_PATCH
  ShellExecute (NULL,
                "open",
                "https://play.google.com/store/apps/details?id=betton.escrime.smartpoule",
                NULL,
                NULL,
                SW_SHOWNORMAL);
#else
  gtk_show_uri (NULL,
                "https://play.google.com/store/apps/details?id=betton.escrime.smartpoule",
                GDK_CURRENT_TIME,
                NULL);
#endif
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_video_released (GtkWidget *widget,
                                                   Object    *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnVideoReleased ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_twitter_id_icon_press (GtkEntry             *entry,
                                                          GtkEntryIconPosition  icon_pos,
                                                          GdkEvent             *event,
                                                          Object               *owner)
{
  Tournament *t = dynamic_cast <Tournament *> (owner);

  t->OnResetTwitterAccount ();
}
