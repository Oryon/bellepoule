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
#include "debug_token_request.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  DebugTokenRequest::DebugTokenRequest (Oauth::Session *session)
    : Object ("Facebook::DebugTokenRequest"),
    Oauth::V2::Request (session, "debug_token", HTTP_GET)
  {
    AddParameterField ("input_token",
                       session->GetToken ());

    {
      gchar *bearer = g_strdup_printf ("%s|%s", session->GetConsumerKey (), session->GetConsumerSecret ());

      SetBearer (bearer);
      g_free (bearer);
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
        {
          GDateTime *date;
          gchar     *expires_string = GetJsonAtPath ("$.data.expires_at");
          guint      expires_int    = g_ascii_strtoull (expires_string,
                                                        nullptr,
                                                        10);

          date = g_date_time_new_from_unix_utc (expires_int);

          {
            gchar *log = g_date_time_format (date, "%c");

            g_print (YELLOW "%s ==>" ESC " %s\n", _session->GetService (), log);
            g_free (log);
          }

          g_date_time_unref (date);
          g_free (expires_string);
        }

        {
          Session *session = dynamic_cast <Session *> (_session);
          gchar   *id      = GetJsonAtPath ("$.data.user_id");

          session->SetUserId (id);
          g_free (id);
        }
      }
    }
  }
}
