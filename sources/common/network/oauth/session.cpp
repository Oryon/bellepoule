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

#include "util/global.hpp"

#include "session.hpp"

namespace Oauth
{
  // --------------------------------------------------------------------------------
  Session::Session (const gchar *service,
                    const gchar *consumer_key,
                    const gchar *consumer_secret)
    : Object ("Oauth::Session")
  {
     _service         = g_strdup (service);
     _consumer_key    = g_strdup (consumer_key);
     _consumer_secret = g_strdup (consumer_secret);
     _signing_key     = g_string_new ("");
     _token           = NULL;

    {
      gchar *token;
      gchar *token_secret;

      token = g_key_file_get_string (Global::_user_config->_key_file,
                                     service,
                                     "token",
                                     NULL);
      token_secret = g_key_file_get_string (Global::_user_config->_key_file,
                                      service,
                                      "token_secret",
                                      NULL);
      SetToken       (token);
      SetTokenSecret (token_secret);

      g_free (token);
      g_free (token_secret);
    }
  }

  // --------------------------------------------------------------------------------
  Session::~Session ()
  {
    g_free (_consumer_key);
    g_free (_consumer_secret);
    g_free (_token);
    g_free (_service);

    g_string_free (_signing_key,
                   TRUE);
  }

  // --------------------------------------------------------------------------------
  void Session::Reset ()
  {
    SetToken       (NULL);
    SetTokenSecret (NULL);
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

    if (token)
    {
      _token = g_strdup (token);
      g_key_file_set_string (Global::_user_config->_key_file,
                             _service,
                             "token",
                             token);
    }
    else
    {
      _token = NULL;
      g_key_file_remove_key (Global::_user_config->_key_file,
                             _service,
                             "token",
                             NULL);
    }
  }

  // --------------------------------------------------------------------------------
  void Session::SetTokenSecret (const gchar *token_secret)
  {
    g_string_free (_signing_key,
                   TRUE);

    _signing_key = g_string_new      (_consumer_secret);
    _signing_key = g_string_append_c (_signing_key,
                                      '&');

    if (token_secret)
    {
      _signing_key = g_string_append (_signing_key,
                                      token_secret);
      g_key_file_set_string (Global::_user_config->_key_file,
                             _service,
                             "token_secret",
                             token_secret);
    }
    else
    {
      g_key_file_remove_key (Global::_user_config->_key_file,
                             _service,
                             "token_secret",
                             NULL);
    }
  }
}
