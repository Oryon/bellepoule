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

#ifndef job_hpp
#define job_hpp

#include "util/object.hpp"

class Job : public Object
{
  public:
    Job (GChecksum *sha1,
         GdkColor  *gdk_color);

    void SetName (const gchar *name);

    gboolean Is (GChecksum *sha1);

    GdkColor *GetGdkColor ();

  private:
    GChecksum *_sha1;
    gchar     *_name;
    GdkColor  *_gdk_color;

    ~Job ();
};

#endif
