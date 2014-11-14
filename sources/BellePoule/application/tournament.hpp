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

#ifndef tournament_hpp
#define tournament_hpp

#include <gtk/gtk.h>

#include "util/module.hpp"
#include "util/glade.hpp"
#include "network/http_server.hpp"
#include "network/downloader.hpp"
#include "network/web_server.hpp"
#include "ecosystem.hpp"

class Contest;

class Tournament : public Module, Net::HttpServer::Client
{
  public:
     Tournament (gchar *filename);

    virtual ~Tournament ();

    static void Init ();

    void OnNew ();

    void OnOpen (gchar *current_folder = NULL);

    void OnRecent ();

    void OnMenuDialog (const gchar *dialog);

    void OnOpenExample ();

    void OnOpenUserManual ();

    void OnOpenTemplate ();

    void OnSave ();

    void PrintMealTickets ();

    void PrintPaymentBook ();

    static gchar *GetUserLanguage ();

    void OnBackupfileLocation ();

    Contest *GetContest (const gchar *filename);

    void Manage (Contest *contest);

    void OnContestDeleted (Contest *contest);

    const gchar *GetBackupLocation ();

    void OnActivateBackup ();

    void OnVideoReleased ();

    void OpenUriContest (const gchar *uri);

    Player *Share (Player  *referee,
                   Contest *from);

    Net::Uploader *GetFtpUpLoader ();

  public:
    const Player *GetPlayer (guint CompetitionId,
                             guint PlayerId);

  private:
    static const guint NB_TICKET_X_PER_SHEET = 2;
    static const guint NB_TICKET_Y_PER_SHEET = 5;
    static const guint NB_REFEREE_PER_SHEET  = 20;

    guint _referee_ref;

    GSList           *_contest_list;
    GSList           *_referee_list;
    guint             _nb_matchs;
    Net::HttpServer  *_http_server;
    Net::Downloader  *_version_downloader;
    Net::WebServer   *_web_server;
    gboolean          _print_meal_tickets;
    EcoSystem        *_ecosystem;

    void SetBackupLocation (gchar *location);

    void EnumerateLanguages ();

    void RefreshMatchRate (gint delta);

    void RefreshMatchRate (Player *referee);

    Contest *FetchContest (const gchar *id);

    Player *UpdateConnectionStatus (GSList      *player_list,
                                    guint        ref,
                                    const gchar *ip_address,
                                    const gchar *status);

    gboolean OnHttpPost (const gchar *data);

    gchar *OnHttpGet (const gchar *url);

    static void OnWebServerState (gboolean  in_progress,
                                  gboolean  on,
                                  Object    *owner);

    guint PreparePrint (GtkPrintOperation *operation,
                        GtkPrintContext   *context);

    void DrawPage (GtkPrintOperation *operation,
                   GtkPrintContext   *context,
                   gint               page_nr);

    gchar *GetSecretKey (const gchar *authentication_scheme);

    static gboolean HttpPostCbk (Net::HttpServer::Client *client,
                                 const gchar             *data);

    static gchar *HttpGetCbk (Net::HttpServer::Client *client,
                              const gchar             *url);

    static void OnLocaleToggled (GtkCheckMenuItem *checkmenuitem,
                                 gchar            *locale);

    static gboolean OnLatestVersionReceived (Net::Downloader::CallbackData *cbk_data);
};

#endif
