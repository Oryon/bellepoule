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

class Contest;

class Tournament : public Module
{
  public:
     Tournament (gchar *filename);

    virtual ~Tournament ();

    static void Init ();

    void OnNew ();

    void OnOpen (gchar *current_folder = NULL);

    void OnRecent ();

    void OnWifi ();

    void OnAbout ();

    void OnOpenExample ();

    void OnOpenUserManual ();

    void OnOpenTemplate ();

    void OnSave ();

    void OnBroadcastedActivated (GtkTreePath *path);

    void PrintMealTickets ();

    void PrintPaymentBook ();

    static gchar *GetUserLanguage ();

    void OnBackupfileLocation ();

    Contest *GetContest (gchar *filename);

    void Manage (Contest *contest);

    void OnContestDeleted (Contest *contest);

    const gchar *GetBackupLocation ();

    void OnActivateBackup ();

    void OpenUriContest (const gchar *uri);

    Player *Share (Player  *referee,
                   Contest *from);

    void GetBroadcastedCompetitions ();

    void StopCompetitionMonitoring ();

  public:
    const gchar *GetCompetitionData (guint  competition_id,
                                     gchar *data_name);

    const Player *GetPlayer (guint CompetitionId,
                             guint PlayerId);

  private:
    static const guint NB_TICKET_X_PER_SHEET = 2;
    static const guint NB_TICKET_Y_PER_SHEET = 5;
    static const guint NB_REFEREE_PER_SHEET  = 20;

    guint _referee_ref;

    GSList          *_contest_list;
    GSList          *_referee_list;
    guint            _nb_matchs;
    Net::HttpServer *_http_server;
    Net::Downloader *_version_downloader;
    Net::Downloader *_competitions_downloader;
    gboolean         _print_meal_tickets;

    void SetBackupLocation (gchar *location);

    void EnumerateLanguages ();

    void RefreshMatchRate (gint delta);

    void RefreshMatchRate (Player *referee);

    gboolean OnHttpPost (const gchar *url,
                         const gchar *data);

    gchar *OnHttpGet (const gchar *url);

    guint PreparePrint (GtkPrintOperation *operation,
                        GtkPrintContext   *context);

    void DrawPage (GtkPrintOperation *operation,
                   GtkPrintContext   *context,
                   gint               page_nr);

    static gboolean HttpPostCbk (Object      *client,
                                 const gchar *data,
                                 const gchar *url);

    static gchar *HttpGetCbk (Object      *client,
                              const gchar *url);

    static void OnLocaleToggled (GtkCheckMenuItem *checkmenuitem,
                                 gchar            *locale);

    static gboolean OnLatestVersionReceived (Net::Downloader::CallbackData *cbk_data);

    static gboolean OnCompetitionListReceived (Net::Downloader::CallbackData *cbk_data);
};

#endif
