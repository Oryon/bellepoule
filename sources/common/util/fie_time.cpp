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

#include <stdlib.h>

#include "fie_time.hpp"

// --------------------------------------------------------------------------------
FieTime::FieTime (GDateTime *time)
  : Object ("FieTime")
{
  _g_date_time = g_date_time_ref (time);
  MakeImages (time);
}

// --------------------------------------------------------------------------------
FieTime::FieTime (const gchar *time)
  : Object ("FieTime")
{
  _image       = nullptr;
  _xml_image   = nullptr;
  _g_date_time = nullptr;

  {
    gchar **tokens = g_strsplit_set (time,
                                     ". :",
                                     0);

    if (tokens)
    {
      guint token_count = 0;

      while (tokens[token_count] != nullptr)
      {
        token_count++;
      }

      if (token_count == 5)
      {
        // JJ.MM.AAAA HH:MI
        _g_date_time = g_date_time_new_local (atoi (tokens[2]),
                                              atoi (tokens[1]),
                                              atoi (tokens[0]),
                                              atoi (tokens[3]),
                                              atoi (tokens[4]),
                                              0.0);
        MakeImages (_g_date_time);
      }

      g_strfreev (tokens);
    }
  }
}

// --------------------------------------------------------------------------------
FieTime::~FieTime ()
{
  g_free (_image);
  g_free (_xml_image);

  if (_g_date_time)
  {
    g_date_time_ref (_g_date_time);
  }
}

// --------------------------------------------------------------------------------
void FieTime::MakeImages (GDateTime *date_time)
{
  _image = g_date_time_format (date_time,
                               "%k:%M");

  _xml_image = g_date_time_format (date_time,
                                   "%d.%m.%Y %R");
}

// --------------------------------------------------------------------------------
GDateTime *FieTime::GetGDateTime ()
{
  return _g_date_time;
}

// --------------------------------------------------------------------------------
const gchar *FieTime::GetImage ()
{
  return _image;
}

// --------------------------------------------------------------------------------
const gchar *FieTime::GetXmlImage ()
{
  return _xml_image;
}
