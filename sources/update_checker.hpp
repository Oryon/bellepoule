// Copyright (C) 2011 Yannick Le Roux.
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

#ifndef update_checker_hpp
#define update_checker_hpp

#include <curl/curl.h>
#include <glib.h>

class UpdateChecker
{
  public:
    static void DownloadLatestVersion (GSourceFunc callback,
                                       gpointer    callback_data);

    static gboolean GetLatestVersion (GKeyFile *key_file);

  private:
    static GSourceFunc  _callback;
    static gpointer     _callback_data;
    static gchar       *_latest;

    struct Data
    {
      char   *text;
      size_t  size;
    };

    static size_t AddText (void   *contents,
                           size_t  size,
                           size_t  nmemb,
                           Data   *data);

    static gpointer ThreadFunction (gpointer thread_data);
};

#endif
