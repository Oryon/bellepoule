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

    SendRequest (new VerifyCredentials (_session));
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

  // --------------------------------------------------------------------------------
  void Twitter::SwitchOn ()
  {
    Advertiser::SwitchOn ();

    SendRequest (new VerifyCredentials (_session));
  }

  // --------------------------------------------------------------------------------
  void Twitter::OnServerResponse (Oauth::V1::Request *request)
  {
    VerifyCredentials *verify_credentials = dynamic_cast <VerifyCredentials *> (request);

    _pending_request = FALSE;

    if (request->GetStatus () == Oauth::V1::Request::NETWORK_ERROR)
    {
      _state = OFF;
      DisplayId ("Network error!");
    }
    else if (request->GetStatus () == Oauth::V1::Request::REJECTED)
    {
      if ((_state == OFF) && verify_credentials)
      {
        _state = WAITING_FOR_TOKEN;
        _session->Reset ();
        SendRequest (new Oauth::V1::RequestToken (_session));
      }
      else
      {
        if (_state != IDLE)
        {
          DisplayId ("Access denied!");
        }
        _state = OFF;
      }
    }
    else
    {
      Oauth::V1::RequestToken *request_token = dynamic_cast <Oauth::V1::RequestToken *> (request);

      if (request_token)
      {
        {
          gchar *pin_url = request_token->GetPinCodeUrl ();

          DisplayAuthorizationPage (pin_url);
          g_free (pin_url);
        }

        if (_pending_request == FALSE)
        {
          SendRequest (new VerifyCredentials (_session));
        }
      }
      else if (dynamic_cast <Oauth::V1::AccessToken *> (request))
      {
        SendRequest (new VerifyCredentials (_session));
      }
      else if (verify_credentials)
      {
        if (_state == IDLE)
        {
          _state = OFF;
        }
        else
        {
          _state = ON;
        }
        DisplayId (verify_credentials->_twitter_account);
      }
    }
  }

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

        for (guint p = 0; params[p] != NULL; p++)
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

      gtk_dialog_response (GTK_DIALOG (_glade->GetWidget ("request_token_dialog")),
                           GTK_RESPONSE_CANCEL);

      webkit_web_policy_decision_ignore (policy_decision);
      return TRUE;
    }

    return FALSE;
  }
}
