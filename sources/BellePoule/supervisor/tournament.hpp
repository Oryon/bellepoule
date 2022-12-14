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

#pragma once

#include "util/module.hpp"
#include "network/ring.hpp"
#include "network/web_server.hpp"
#include "network/usb_broker.hpp"

class Contest;
class Publication;
class Glade;
class Player;

class Tournament : public Module,
                   public Net::Ring::Listener,
                   public Net::WebServer::Listener
{
  public:
    Tournament ();

    static void Init ();

    void Start (gchar *filename);

    void OnNew ();

    void OnOpen (gchar       *current_folder,
                 const gchar *filter_suffix);

    void OnRecent ();

    void OnReload ();

    void OnMenuDialog (const gchar *dialog);

    void OnOpenExample ();

    void OnOpenTemplate ();

    void OnSave ();

    void OnBackupfileLocation ();

    Contest *GetContest (const gchar *filename);

    void Manage (Contest *contest);

    void OnContestDeleted (Contest *contest);

    const gchar *GetBackupLocation ();

    void OnActivateBackup ();

    void OnVideoReleased ();

    void OpenUriContest (const gchar *uri);

    void OnShowAccessCode (gboolean with_steps);

    Publication *GetPublication ();

    gboolean OnMessage (Net::Message *message) override;

    const gchar *GetSecretKey (const gchar *authentication_scheme) override;

    void CreateNewContest (GtkWidget *from);

  public:
    const Player *GetPlayer (guint CompetitionId,
                             guint PlayerId);

  private:
    static Tournament *_singleton;

    GList          *_contest_list;
    Net::WebServer *_web_server;
    Publication    *_publication;
    GList          *_advertisers;

    ~Tournament () override;

    Contest *GetContest (guint netid);

    void SetBackupLocation (gchar *location);

    void OnHanshakeResult (Net::Ring::HandshakeResult result) override;

    void OnUsbEvent (Net::UsbBroker::Event  event,
                     Net::UsbDrive         *drive) override;

    static gboolean RecentFileExists (const GtkRecentFilterInfo *filter_info,
                                      Tournament                *tournament);

    void OnWebServerState (gboolean in_progress,
                           gboolean on) override;

    GtkFileFilter *GetFileFilter (const gchar *name,
                                  ...);
};
