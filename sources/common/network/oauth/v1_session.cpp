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

#include "v1_session.hpp"

namespace Oauth
{
  namespace V1
  {
    // --------------------------------------------------------------------------------
    Session::Session (const gchar *service,
                      const gchar *api_uri,
                      const gchar *consumer_key,
                      const gchar *consumer_secret)
      : Object ("Oauth::V1::Session"),
        Oauth::Session (service,
                        api_uri,
                        consumer_key,
                        consumer_secret)
    {
      _signing_key = g_string_new ("");

      {
        gchar *token_secret = g_key_file_get_string (Global::_user_config->_key_file,
                                                     service,
                                                     "token_secret",
                                                     NULL);

        SetTokenSecret (token_secret);
        g_free (token_secret);
      }
    }

    // --------------------------------------------------------------------------------
    Session::~Session ()
    {
      g_string_free (_signing_key,
                     TRUE);
    }

    // --------------------------------------------------------------------------------
    void Session::Reset ()
    {
      Oauth::Session::Reset ();
      SetTokenSecret (NULL);
    }

    // --------------------------------------------------------------------------------
    const guchar *Session::GetSigningKey ()
    {
      return (guchar *) _signing_key->str;
    }

    // --------------------------------------------------------------------------------
    void Session::SetTokenSecret (const gchar *token_secret)
    {
      g_string_free (_signing_key,
                     TRUE);

      _signing_key = g_string_new      (GetConsumerSecret ());
      _signing_key = g_string_append_c (_signing_key,
                                        '&');

      if (token_secret)
      {
        _signing_key = g_string_append (_signing_key,
                                        token_secret);
        g_key_file_set_string (Global::_user_config->_key_file,
                               GetService (),
                               "token_secret",
                               token_secret);
      }
      else
      {
        g_key_file_remove_key (Global::_user_config->_key_file,
                               GetService (),
                               "token_secret",
                               NULL);
      }
    }
  }
}
