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

#include "oauth/session.hpp"
#include "debug_token_request.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  DebugTokenRequest::DebugTokenRequest (Oauth::Session *session)
    : Object ("Facebook::DebugTokenRequest"),
    Request (session, "debug_token", GET)
  {
    AddParameterField ("input_token",
                       session->GetToken ());

    {
      gchar *access = g_strdup_printf ("%s|%s", session->GetConsumerKey (), session->GetConsumerSecret ());

      AddParameterField ("access_token",
                         access);
      g_free (access);
    }
  }

  // --------------------------------------------------------------------------------
  DebugTokenRequest::~DebugTokenRequest ()
  {
  }

  // --------------------------------------------------------------------------------
  void DebugTokenRequest::ParseResponse (GHashTable  *header,
                                         const gchar *body)
  {
    Oauth::Request::ParseResponse (header,
                                   body);

    if (GetStatus () == ACCEPTED)
    {
      if (LoadJson (body))
      {
        printf ("===> %s\n", GetJsonAtPath ("$.data.expires_at"));
        printf ("===> %s\n", GetJsonAtPath ("$.data.user_id"));
      }
    }
  }
}
