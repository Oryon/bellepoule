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

#include "util/object.hpp"
#include "network/downloader.hpp"
#include "network/http_server.hpp"

class Attribute;
class AttributeDesc;
class Application;
class Module;
class Language;

class Application :
  public Object,
         Net::HttpServer::Client,
         Net::Downloader::Listener
{
  public:
    virtual void Prepare ();

    virtual void Start (int argc, char **argv);

    void OnOpenUserManual ();

    virtual void OnQuit (GtkWindow *window);

  protected:
    Module *_main_module;

    Application (const gchar   *role,
                 guint          http_port,
                 int           *argc,
                 char        ***argv);

    virtual ~Application ();

    virtual gboolean OnHttpPost (Net::Message *message);

    virtual gchar *OnHttpGet (const gchar *url);

  private:
    gchar           *_role;
    Language        *_language;
    Net::Downloader *_version_downloader;
    Net::HttpServer *_http_server;

    void OnDownloaderData (Net::Downloader  *downloader,
                           const gchar      *data);

    virtual gchar *GetSecretKey (const gchar *authentication_scheme);

    static void AboutDialogActivateLinkFunc (GtkAboutDialog *about,
                                             const gchar    *link,
                                             gpointer        data);

    static gint CompareRanking (Attribute *attr_a,
                                Attribute *attr_b);

    static gint CompareDate (Attribute *attr_a,
                             Attribute *attr_b);

    static void LogHandler (const gchar    *log_domain,
                            GLogLevelFlags  log_level,
                            const gchar    *message,
                            gpointer        user_data);

    static gboolean IsCsvReady (AttributeDesc *desc);

    static gboolean HttpPostCbk (Net::HttpServer::Client *client,
                                 Net::Message            *message);

    static gchar *HttpGetCbk (Net::HttpServer::Client *client,
                              const gchar             *url);
};
