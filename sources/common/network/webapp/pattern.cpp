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

#include "pattern.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  Pattern::Pattern (const gchar *regexp,
                    const gchar *error_msg)
    : Object ("Pattern")
  {
    _error_msg = error_msg;
    _argument  = 0;

    _pattern = g_regex_new (regexp,
                            G_REGEX_OPTIMIZE,
                            (GRegexMatchFlags) 0,
                            nullptr);
  }

  // --------------------------------------------------------------------------------
  Pattern::~Pattern ()
  {
    g_regex_unref (_pattern);
  }

  // --------------------------------------------------------------------------------
  gboolean Pattern::Match (const gchar *path)
  {
    gboolean    match;
    GMatchInfo *match_info;

    match = g_regex_match (_pattern,
                           path,
                           (GRegexMatchFlags) G_REGEX_ANCHORED,
                           &match_info);

    if (match)
    {
      gchar *info = g_match_info_fetch (match_info, 1);

      if (info)
      {
        _argument = g_ascii_strtoull (info,
                                      nullptr,
                                      10);

        g_free (info);
      }
    }

    g_match_info_free (match_info);

    return match;
  }

  // --------------------------------------------------------------------------------
  guint Pattern::GetArgument ()
  {
    return _argument;
  }

  // --------------------------------------------------------------------------------
  gchar *Pattern::GetErrorString ()
  {
    gchar *error_string = nullptr;

    if (_error_msg)
    {
      GString *error = g_string_new (nullptr);

      g_string_append        (error, "<meta charset=\"UTF-8\">");
      g_string_append_printf (error, _error_msg, _argument);

      error_string = error->str;
      g_string_free (error,
                     FALSE);
    }

    return error_string;
  }
}
