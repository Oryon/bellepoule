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
#include "advertiser_ids.hpp"

#include "facebook.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  Facebook::Facebook ()
    : Object ("Facebook"),
      Advertiser ("facebook")
  {
    Oauth::Session *session = new Oauth::Session (_name,
                                                  "https://www.facebook.com",
                                                  FACEBOOK_CONSUMER_KEY);

    SetSession (session);
  }

  // --------------------------------------------------------------------------------
  Facebook::~Facebook ()
  {
  }

  // --------------------------------------------------------------------------------
  void Facebook::SwitchOn ()
  {
    Advertiser::SwitchOn ();

    {
      gchar *url = g_strdup_printf ("https://www.facebook.com/v2.8/dialog/oauth"
                                    "?client_id=%s"
                                    "&redirect_uri=https://www.facebook.com/connect/login_success.html", _session->GetConsumerKey ());

      DisplayAuthorizationPage (url);
      g_free (url);
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Facebook::OnRedirect (WebKitNetworkRequest    *request,
                                 WebKitWebPolicyDecision *policy_decision)
  {
    const gchar *request_uri = webkit_network_request_get_uri (request);

    printf (YELLOW "%s\n" ESC, request_uri);
    if (g_strrstr (request_uri, "https://www.facebook.com/connect/login_success.html?"))
    {
      {
        gchar **params = g_strsplit_set (request_uri,
                                         "?&",
                                         -1);

        for (guint p = 0; params[p] != NULL; p++)
        {
          if (g_strrstr (params[p], "code="))
          {
            gchar **code = g_strsplit_set (params[p],
                                           "=",
                                           -1);

            if (code[0] && code[1])
            {
              printf (BLUE "    %s\n", code[1]);
              _session->SetToken (code[1]);
            }
            g_strfreev (code);
            break;
          }
        }

        g_strfreev (params);
      }

      gtk_dialog_response (GTK_DIALOG (_glade->GetWidget ("request_token_dialog")),
                           GTK_RESPONSE_CANCEL);

      webkit_web_policy_decision_ignore (policy_decision);
      return TRUE;
    }

    return FALSE;
  }
}
