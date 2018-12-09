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

#include "error.hpp"

// --------------------------------------------------------------------------------
Error::Error (Level        level,
              const gchar *guilty_party,
              const gchar *reason)
  : Object ("Error")
{
  _level        = level;
  _guilty_party = nullptr;
  _reason       = nullptr;

  if (guilty_party)
  {
    _guilty_party = g_strdup (guilty_party);
  }

  if (reason)
  {
    _reason = g_strdup (reason);
  }
}

// --------------------------------------------------------------------------------
Error::~Error ()
{
  g_free (_guilty_party);
  g_free (_reason);
}

// --------------------------------------------------------------------------------
gchar *Error::GetText ()
{
  if (_guilty_party)
  {
    return g_strdup_printf (" <span foreground=\"black\" weight=\"800\">%s:</span> \n"
                            " <span foreground=\"black\" weight=\"400\">%s</span> ",
                            _guilty_party,
                            _reason);
  }
  else
  {
    return g_strdup_printf (" <span foreground=\"black\" weight=\"400\">%s</span> ",
                            _reason);
  }
}

// --------------------------------------------------------------------------------
const gchar *Error::GetColor ()
{
  if (_level == Level::WARNING)
  {
    return "#e3d42b";
  }
  else
  {
    return "#d52323";
  }
}
