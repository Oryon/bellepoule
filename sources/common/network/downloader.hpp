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

#pragma once

#include <curl/curl.h>
#include <glib.h>

#include "util/object.hpp"

namespace Net
{
  class Downloader : public Object
  {
    public:
      struct Listener
      {
        virtual void OnDownloaderData (Downloader  *downloader,
                                       const gchar *data) = 0;
      };

    public:
      Downloader (const gchar *name,
                  Listener    *listener);

      void Start (const gchar *address);

      void Kill ();

    private:
      Listener *_listener;
      GThread  *_thread;
      gchar    *_data;
      size_t    _size;
      gchar    *_address;
      gboolean  _killed;
      gchar    *_name;

      static size_t AddText (void   *contents,
                             size_t  size,
                             size_t  nmemb,
                             Downloader *downloader);

      static gpointer ThreadFunction (Downloader *downloader);

      static gboolean OnThreadDone (Downloader *downloader);

      ~Downloader () override;
  };
}
