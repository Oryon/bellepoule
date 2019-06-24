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
FieTime::FieTime (GDateTime *datetime)
  : Object ("FieTime")
{
  _g_date_time = g_date_time_ref (datetime);
  MakeImages (datetime);
}

// --------------------------------------------------------------------------------
FieTime::FieTime (const gchar *date,
                  const gchar *time)
  : Object ("FieTime")
{
  gint day    = -1;
  gint month  = -1;
  gint year   = -1;
  gint hour   = -1;
  gint minute = -1;

  _image       = nullptr;
  _xml_date    = nullptr;
  _xml_time    = nullptr;
  _g_date_time = nullptr;

  {
    gchar **tokens = g_strsplit_set (date,
                                     ".",
                                     0);

    if (tokens)
    {
      guint token_count = 0;

      while (tokens[token_count] != nullptr)
      {
        token_count++;
      }

      if (token_count == 3)
      {
        // JJ.MM.AAAA
         year  = atoi (tokens[2]);
         month = atoi (tokens[1]);
         day   = atoi (tokens[0]);
      }

      g_strfreev (tokens);
    }
  }

  {
    gchar **tokens = g_strsplit_set (time,
                                     ":",
                                     0);

    if (tokens)
    {
      guint token_count = 0;

      while (tokens[token_count] != nullptr)
      {
        token_count++;
      }

      if (token_count == 2)
      {
        // HH:MI
        hour   = atoi (tokens[0]);
        minute = atoi (tokens[1]);
      }

      g_strfreev (tokens);
    }
  }

  if ((year != -1) && (month != -1) && (day != -1) && (hour != -1) && (minute != -1))
  {
    _g_date_time = g_date_time_new_local (year,
                                          month,
                                          day,
                                          hour,
                                          minute,
                                          0.0);
    MakeImages (_g_date_time);
  }
}

// --------------------------------------------------------------------------------
FieTime::~FieTime ()
{
  g_free (_image);
  g_free (_xml_date);
  g_free (_xml_time);

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

  _xml_date = g_date_time_format (date_time,
                                  "%d.%m.%Y");
  _xml_time = g_date_time_format (date_time,
                                  "%R");
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
const gchar *FieTime::GetXmlDate ()
{
  return _xml_date;
}

// --------------------------------------------------------------------------------
const gchar *FieTime::GetXmlTime ()
{
  return _xml_time;
}

// --------------------------------------------------------------------------------
gint FieTime::Compare (FieTime *A,
                       FieTime *B)
{
  if ((A && A->_g_date_time) && (B && B->_g_date_time))
  {
    return g_date_time_compare (A->_g_date_time,
                                B->_g_date_time);
  }
  else if (A && A->_g_date_time)
  {
    return 1;
  }
  else if (B && B->_g_date_time)
  {
    return -1;
  }

  return 0;
}
