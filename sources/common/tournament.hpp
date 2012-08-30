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

#include "module.hpp"
#include "glade.hpp"
#include "http_server.hpp"
#include "downloader.hpp"

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

    GSList     *_contest_list;
    GSList     *_referee_list;
    guint       _nb_matchs;
    HttpServer *_http_server;
    Downloader *_version_downloader;
    Downloader *_competitions_downloader;
    gboolean    _print_meal_tickets;

    void SetBackupLocation (gchar *location);

    void EnumerateLanguages ();

    void RefreshMatchRate (gint delta);

    void RefreshMatchRate (Player *referee);

    gchar *GetHttpResponse (const gchar *url);

    void OnBeginPrint (GtkPrintOperation *operation,
                       GtkPrintContext   *context);

    void OnDrawPage (GtkPrintOperation *operation,
                     GtkPrintContext   *context,
                     gint               page_nr);

    static gchar *OnGetHttpResponse (Object      *client,
                                     const gchar *url);

    static void OnLocaleToggled (GtkCheckMenuItem *checkmenuitem,
                                 gchar            *locale);

    static gboolean OnLatestVersionReceived (Downloader::CallbackData *cbk_data);

    static gboolean OnCompetitionListReceived (Downloader::CallbackData *cbk_data);
};

#endif
