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
  Pattern::Pattern (const gchar *path,
                    const gchar *description,
                    const gchar *uri_path_error)
    : Object ("Pattern")
  {
     _argument       = 0;
     _description    = description;
     _uri_path_error = uri_path_error;

     _scheme = g_string_new (path);

     {
       GString  *full_path     = g_string_new (path);
       gchar    *regexp;
       gchar   **path_sections;

       if (uri_path_error)
       {
         g_string_append (full_path, "/([0-9]+)");
         g_string_append (_scheme, "/<strong><span style=\"color: #0000ff;\">XX</span></strong>");
       }
       g_string_append (full_path, "$");

       path_sections = g_strsplit_set (full_path->str,
                                       "/",
                                       0);

       regexp = g_strjoinv ("\\/",
                            path_sections);

       _pattern = g_regex_new (regexp,
                               G_REGEX_OPTIMIZE,
                               (GRegexMatchFlags) 0,
                               nullptr);

       g_free (regexp);
       g_strfreev (path_sections);
       g_string_free (full_path,
                      TRUE);
     }
  }

  // --------------------------------------------------------------------------------
  Pattern::~Pattern ()
  {
    g_regex_unref (_pattern);
    g_string_free (_scheme,
                   TRUE);
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
  const gchar *Pattern::GetScheme ()
  {
    return _scheme->str;
  }

  // --------------------------------------------------------------------------------
  const gchar *Pattern::GetDescription ()
  {
    return _description;
  }

  // --------------------------------------------------------------------------------
  gchar *Pattern::GetUriPathError ()
  {
    gchar *error = nullptr;

    if (_uri_path_error)
    {
      GString *error_string = g_string_new (nullptr);

      g_string_append        (error_string, "<meta charset=\"UTF-8\">");
      g_string_append_printf (error_string, _uri_path_error, _argument);

      error = error_string->str;

      g_string_free (error_string,
                     FALSE);
    }

    return error;
  }
}
