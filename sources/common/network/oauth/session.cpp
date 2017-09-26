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
                    const gchar *api_uri,
                    const gchar *consumer_key,
                    const gchar *consumer_secret)
    : Object ("Oauth::Session")
  {
    _api_uri            = g_strdup (api_uri);
    _service            = g_strdup (service);
    _consumer_key       = g_strdup (consumer_key);
    _consumer_secret    = g_strdup (consumer_secret);
    _token              = NULL;
    _account_id         = NULL;
    _authorization_page = NULL;

    {
      gchar *token = g_key_file_get_string (Global::_user_config->_key_file,
                                            service,
                                            "token",
                                            NULL);
      SetToken (token);
      g_free (token);
    }
  }

  // --------------------------------------------------------------------------------
  Session::~Session ()
  {
    g_free (_api_uri);
    g_free (_service);
    g_free (_consumer_key);
    g_free (_consumer_secret);
    g_free (_token);
    g_free (_account_id);
    g_free (_authorization_page);
  }

  // --------------------------------------------------------------------------------
  void Session::Reset ()
  {
    SetToken (NULL);

    g_free (_account_id);
    _account_id = NULL;
  }

  // --------------------------------------------------------------------------------
  const gchar *Session::GetApiUri ()
  {
    return _api_uri;
  }

  // --------------------------------------------------------------------------------
  const gchar *Session::GetConsumerKey ()
  {
    return _consumer_key;
  }

  // --------------------------------------------------------------------------------
  const gchar *Session::GetService ()
  {
    return _service;
  }

  // --------------------------------------------------------------------------------
  const gchar *Session::GetConsumerSecret ()
  {
    return _consumer_secret;
  }

  // --------------------------------------------------------------------------------
  const gchar *Session::GetToken ()
  {
    return _token;
  }

  // --------------------------------------------------------------------------------
  void Session::SetAccountId (const gchar *id)
  {
    g_free (_account_id);
    _account_id = g_strdup (id);
  }

  // --------------------------------------------------------------------------------
  const gchar *Session::GetAccountId ()
  {
    return _account_id;
  }

  // --------------------------------------------------------------------------------
  void Session::SetAuthorizationPage (const gchar *id)
  {
    g_free (_authorization_page);
    _authorization_page = g_strdup (id);
  }

  // --------------------------------------------------------------------------------
  const gchar *Session::GetAuthorizationPage ()
  {
    return _authorization_page;
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
}
