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
#include "network/ring.hpp"
#include "network/downloader.hpp"

class Attribute;
class AttributeDesc;
class Module;
class Language;

namespace Net
{
  class Message;
}

class Application :
  public Object,
  public Net::Downloader::Listener
{
  public:
    virtual void Prepare ();

    virtual void Start (int                   argc,
                        char                **argv,
                        Net::Ring::Listener  *ring_listener);

    void OnOpenUserManual ();

    virtual void OnQuit (GtkWindow *window);

  protected:
    Module *_main_module;

    Application (Net::Ring::Role     role,
                 const gchar        *public_name,
                 int                *argc,
                 char            ***argv);

    ~Application () override;

  private:
    Net::Ring::Role  _role;
    Language        *_language;
    Net::Downloader *_version_downloader;

    void OnDownloaderData (Net::Downloader  *downloader,
                           const gchar      *data) override;

    virtual const gchar *GetSecretKey (const gchar *authentication_scheme);

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
};
