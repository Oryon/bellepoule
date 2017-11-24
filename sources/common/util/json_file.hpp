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

#include <json-glib/json-glib.h>

#include "util/object.hpp"

class JsonFile : public Object
{
  public:
    struct Listener
    {
      virtual void FeedJsonBuilder (JsonBuilder *builder) = 0;
      virtual gboolean ReadJson    (JsonReader  *reader)  = 0;
    };

  public:
    JsonFile (Listener    *listener,
              const gchar *path);

    gboolean Load ();

    void MakeDirty ();

    void Save ();

  private:
    Listener *_listener;
    guint     _tag;
    gchar    *_path;

    ~JsonFile ();

    static gboolean OnTimeout (JsonFile *json_file);
};
