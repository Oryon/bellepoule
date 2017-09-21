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

#include "verify_credentials_request.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  VerifyCredentials::VerifyCredentials (Oauth::Session *session)
    : Object ("Twitter::VerifyCredentials"),
              Oauth::HttpRequest (session, "1.1/account/verify_credentials.json", GET)
  {
    _twitter_account = NULL;
  }

  // --------------------------------------------------------------------------------
  VerifyCredentials::~VerifyCredentials ()
  {
    g_free (_twitter_account);
  }

  // --------------------------------------------------------------------------------
  void VerifyCredentials::ParseResponse (GHashTable  *header,
                                         const gchar *body)
  {
    HttpRequest::ParseResponse (header,
                                body);

    if (GetStatus () == Oauth::HttpRequest::ACCEPTED)
    {
      if (LoadJson (body))
      {
        char *name        = GetJsonAtPath ("$.name");
        char *screen_name = GetJsonAtPath ("$.screen_name");

        if (name && screen_name)
        {
          _twitter_account = g_strdup_printf ("%s@%s", name, screen_name);
        }

        g_free (screen_name);
        g_free (name);
      }
    }
  }
}
