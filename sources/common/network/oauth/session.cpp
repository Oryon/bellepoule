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

#include "session.hpp"

namespace Oauth
{
  // --------------------------------------------------------------------------------
  Session::Session (const gchar *consumer_key,
                    const gchar *consumer_secret)
    : Object ("Oauth::Session")
  {
    _consumer_key    = g_strdup (consumer_key);
    _consumer_secret = g_strdup (consumer_secret);

    _signing_key = NULL;

    _token = NULL;
    SetTokenSecret ("");
  }

  // --------------------------------------------------------------------------------
  Session::~Session ()
  {
    g_free (_consumer_key);
    g_free (_consumer_secret);
    g_free (_token);

    g_string_free (_signing_key,
                   TRUE);
  }

  // --------------------------------------------------------------------------------
  const gchar *Session::GetConsumerKey ()
  {
    return _consumer_key;
  }

  // --------------------------------------------------------------------------------
  const gchar *Session::GetToken ()
  {
    return _token;
  }

  // --------------------------------------------------------------------------------
  const guchar *Session::GetSigningKey ()
  {
    return (guchar *) _signing_key->str;
  }

  // --------------------------------------------------------------------------------
  void Session::SetToken (const gchar *token)
  {
    g_free (_token);
    _token = g_strdup (token);
  }

  // --------------------------------------------------------------------------------
  void Session::SetTokenSecret (const gchar *token_secret)
  {
    if (_consumer_secret)
    {
      if (_signing_key)
      {
        g_string_free (_signing_key,
                       TRUE);
      }

      _signing_key = g_string_new (_consumer_secret);
      _signing_key = g_string_append_c (_signing_key,
                                        '&');
      _signing_key = g_string_append (_signing_key,
                                      token_secret);
    }
  }
}
