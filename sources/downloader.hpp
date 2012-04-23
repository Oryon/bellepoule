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

#ifndef downloader_hpp
#define downloader_hpp

#include <curl/curl.h>
#include <glib.h>

#include "object.hpp"

class Downloader : public Object
{
  public:
    Downloader (GSourceFunc callback,
                gpointer    callback_data);

    void Start (const gchar *address);

    gchar *GetData ();

  private:
    GSourceFunc  _callback;
    gpointer     _callback_data;
    gchar       *_data;
    size_t       _size;
    gchar       *_address;

    static size_t AddText (void   *contents,
                           size_t  size,
                           size_t  nmemb,
                           Downloader *downloader);

    static gpointer ThreadFunction (Downloader *downloader);

    ~Downloader ();
};

#endif
