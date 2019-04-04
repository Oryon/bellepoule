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

#include "source.hpp"

namespace People
{
  // --------------------------------------------------------------------------------
  Source::Source ()
    : Object ("Source")
  {
    _url  = nullptr;
    _sha1 = nullptr;
  }

  // --------------------------------------------------------------------------------
  gchar *Source::Serialize ()
  {
    gchar *result = nullptr;

    if (_sha1 && _url)
    {
      GString *string = g_string_new (_sha1);

      g_string_append_c (string, '-');
      g_string_append   (string, _url);

      result = string->str;

      g_string_free (string,
                     FALSE);
    }

    return result;
  }

  // --------------------------------------------------------------------------------
  void Source::Load (const gchar *data)
  {
    if (data)
    {
      gchar **tokens = g_strsplit_set (data,
                                       "-",
                                       2);

      if (tokens[0] && tokens[1])
      {
        _sha1 = g_strdup (tokens[0]);
        _url  = g_strdup (tokens[1]);
      }

      g_strfreev (tokens);
    }
  }

  // --------------------------------------------------------------------------------
  Source::~Source ()
  {
    g_free (_url);
    g_free (_sha1);
  }

  // --------------------------------------------------------------------------------
  gchar *Source::ComputeSha1 (const gchar *url)
  {
    gchar *sha1     = nullptr;
    gchar *contents;

    if (g_file_get_contents (url,
                             &contents,
                             nullptr,
                             nullptr))
    {
      sha1 = g_compute_checksum_for_string (G_CHECKSUM_SHA1,
                                            contents,
                                            -1);
      g_free (contents);
    }

    return sha1;
  }

  // --------------------------------------------------------------------------------
  const gchar *Source::GetUrl ()
  {
    return _url;
  }

  // --------------------------------------------------------------------------------
  void Source::SetUrl (const gchar *url)
  {
    g_free (_sha1);
    _sha1 = nullptr;

    if ((_url == nullptr) || (g_strcmp0 (_url, url) == 0))
    {
      _sha1 = ComputeSha1 (url);

      if (_sha1)
      {
        _url = g_strdup (url);
      }
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Source::HasUpdates ()
  {
    gboolean has_updates = FALSE;

    if (_sha1)
    {
      gchar *new_sha1 = ComputeSha1 (_url);

      if (new_sha1)
      {
        has_updates = g_strcmp0 (_sha1, new_sha1) != 0;
        g_free (new_sha1);
      }
    }

    return has_updates;
  }
}
