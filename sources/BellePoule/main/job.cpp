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

#include "job.hpp"

// --------------------------------------------------------------------------------
Job::Job (GChecksum *sha1,
          GdkColor  *gdk_color)
  : Object ("Job")
{
  _sha1      = g_checksum_copy (sha1);
  _gdk_color = gdk_color_copy  (gdk_color);
  _name      = NULL;
}

// --------------------------------------------------------------------------------
Job::~Job ()
{
  g_checksum_free (_sha1);
  g_free          (_name);
  gdk_color_free  (_gdk_color);
}

// --------------------------------------------------------------------------------
void Job::SetName (const gchar *name)
{
  _name = g_strdup (name);
}

// --------------------------------------------------------------------------------
gboolean Job::Is (GChecksum *sha1)
{
  return (strcmp (g_checksum_get_string (sha1),
                  g_checksum_get_string (_sha1)) == 0);
}

// --------------------------------------------------------------------------------
GdkColor *Job::GetGdkColor ()
{
  return _gdk_color;
}
