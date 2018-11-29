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

#include "util/glade.hpp"
#include "oauth/session.hpp"
#include "oauth/v1_request_token.hpp"
#include "oauth/v1_access_token.hpp"
#include "oauth/v1_session.hpp"
#include "verify_credentials_request.hpp"
#include "statuses_update_request.hpp"
#include "advertiser_ids.hpp"

#include "twitter.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  Twitter::Twitter ()
    : Object ("Twitter"),
      Advertiser ("twitter")
  {
    Oauth::Session *session = new Oauth::V1::Session (_name,
                                                      "https://api.twitter.com",
                                                      TWITTER_CONSUMER_KEY,
                                                      TWITTER_CONSUMER_SECRET);

    SetSession (session);
  }

  // --------------------------------------------------------------------------------
  Twitter::~Twitter ()
  {
  }

  // --------------------------------------------------------------------------------
  void Twitter::PublishMessage (const gchar *message)
  {
    SendRequest (new StatusesUpdate (_session,
                                     message));
  }

#ifdef WEBKIT
  // --------------------------------------------------------------------------------
  gboolean Twitter::OnRedirect (WebKitNetworkRequest    *request,
                                WebKitWebPolicyDecision *policy_decision)
  {
    const gchar *request_uri = webkit_network_request_get_uri (request);

    if (g_strrstr (request_uri, "http://bellepoule.bzh"))
    {
      {
        gchar **params = g_strsplit_set (request_uri,
                                         "?&",
                                         -1);

        for (guint p = 0; params[p] != nullptr; p++)
        {
          if (g_strrstr (params[p], "oauth_verifier="))
          {
            gchar **verifyer = g_strsplit_set (params[p],
                                               "=",
                                               -1);

            if (verifyer[0] && verifyer[1])
            {
              SendRequest (new Oauth::V1::AccessToken (_session,
                                                       verifyer[1]));
            }
            g_strfreev (verifyer);
            break;
          }
        }

        g_strfreev (params);
      }

      gtk_dialog_response (GTK_DIALOG (_glade->GetWidget ("webkit_dialog")),
                           GTK_RESPONSE_CANCEL);

      webkit_web_policy_decision_ignore (policy_decision);
      return TRUE;
    }

    return FALSE;
  }
#endif

  // --------------------------------------------------------------------------------
  gboolean Twitter::HandleRequestResponse (Oauth::Request *request)
  {
    Oauth::V1::RequestToken *request_token = dynamic_cast <Oauth::V1::RequestToken *> (request);

    if (request_token)
    {
      gchar *pin_url = request_token->GetPinCodeUrl ();

      _session->SetAuthorizationPage (pin_url);
      DisplayAuthorizationPage ();
      g_free (pin_url);
    }
    else if (dynamic_cast <Oauth::V1::AccessToken *> (request))
    {
      CheckAuthorization ();
    }
    else
    {
      return FALSE;
    }

    return TRUE;
  }

  // --------------------------------------------------------------------------------
  void Twitter::CheckAuthorization ()
  {
    SendRequest (new VerifyCredentials (_session));
  }

  // --------------------------------------------------------------------------------
  void Twitter::ClaimForAuthorization ()
  {
    SendRequest (new Oauth::V1::RequestToken (_session, _oob_authentication));
  }
}
