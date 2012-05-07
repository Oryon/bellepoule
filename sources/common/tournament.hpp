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

    ~Tournament ();

    static void Init ();

    void OnNew ();

    void OnOpen (gchar *current_folder = NULL);

    void OnRecent ();

    void OnWifi ();

    void OnAbout ();

    void OnOpenExample ();

    void OnOpenUserManual ();

    void OnSave ();

    void OnBroadcastedActivated (GtkTreePath *path);

    static gchar *GetUserLanguage ();

    void OnBackupfileLocation ();

    Contest *GetContest (gchar *filename);

    void Manage (Contest *contest);

    void OnContestDeleted (Contest *contest);

    const gchar *GetBackupLocation ();

    void OnActivateBackup ();

    void OpenUriContest (const gchar *uri);

    void OpenMemoryContest (const gchar *memory);

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
    guint _referee_ref;

    GSList     *_contest_list;
    GSList     *_referee_list;
    guint       _nb_matchs;
    HttpServer *_http_server;
    Downloader *_version_downloader;
    Downloader *_competitions_downloader;

    void SetBackupLocation (gchar *location);

    void EnumerateLanguages ();

    void RefreshMatchRate (gint delta);

    void RefreshMatchRate (Player *referee);

    gchar *GetHttpResponse (const gchar *url);

    static gchar *OnGetHttpResponse (Object      *client,
                                     const gchar *url);

    static void OnLocaleToggled (GtkCheckMenuItem *checkmenuitem,
                                 gchar            *locale);

    static gboolean OnLatestVersionReceived (Downloader::CallbackData *cbk_data);

    static gboolean OnCompetitionListReceived (Downloader::CallbackData *cbk_data);

    static gboolean OnCompetitionReceived (Downloader::CallbackData *cbk_data);
};

#endif
