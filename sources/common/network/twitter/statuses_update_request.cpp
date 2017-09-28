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

#include "statuses_update_request.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  StatusesUpdate::StatusesUpdate (Oauth::Session *session,
                                  const gchar     *tweet)
    : Object ("Twitter::StatusesUpdate"),
      Oauth::V1::Request (session, "1.1/statuses/update.json", HTTP_POST)
  {
    AddParameterField ("status", tweet);
  }

  // --------------------------------------------------------------------------------
  StatusesUpdate::~StatusesUpdate ()
  {
  }

  // --------------------------------------------------------------------------------
  void StatusesUpdate::ParseResponse (GHashTable  *header,
                                      const gchar *body)
  {
    Oauth::V1::Request::ParseResponse (header,
                                       body);

    if (GetStatus () == REJECTED)
    {
      if (LoadJson (body))
      {
        char *code = GetJsonAtPath ("$.errors[0].code");

        if (code && g_strcmp0 (code, "187") == 0)
        {
          printf ("Error << " RED "%s" ESC " >> dropped\n", code);
          ForgiveError ();
        }

        g_free (code);
      }
    }
  }
}
