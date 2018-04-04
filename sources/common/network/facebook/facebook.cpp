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
#include "session.hpp"
#include "advertiser_ids.hpp"
#include "debug_token_request.hpp"
#include "feed_request.hpp"
#include "me_request.hpp"
#include "revoke_request.hpp"

#include "facebook.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  Facebook::Facebook ()
    : Object ("Facebook"),
      Advertiser ("facebook")
  {
    Oauth::Session *session = new Session (_name,
                                           "https://graph.facebook.com/v2.10",
                                           FACEBOOK_CONSUMER_KEY,
                                           FACEBOOK_CONSUMER_SECRET);

    {
      gchar *url = g_strdup_printf ("https://www.facebook.com/v2.10/dialog/oauth"
                                    "?client_id=%s"
                                    "&redirect_uri=https://www.facebook.com/connect/login_success.html"
                                    "&response_type=token"
                                    "&scope=publish_actions",
                                    session->GetConsumerKey ());

      session->SetAuthorizationPage (url);
      g_free (url);
    }

    SetSession (session);
  }

  // --------------------------------------------------------------------------------
  Facebook::~Facebook ()
  {
  }

  // --------------------------------------------------------------------------------
  void Facebook::PublishMessage (const gchar *message)
  {
    SendRequest (new FeedRequest (_session,
                                  message));
  }

  // --------------------------------------------------------------------------------
  void Facebook::CheckAuthorization ()
  {
    SendRequest (new DebugTokenRequest (_session));
  }

  // --------------------------------------------------------------------------------
  void Facebook::ClaimForAuthorization ()
  {
    DisplayAuthorizationPage ();
  }

  // --------------------------------------------------------------------------------
  void Facebook::Reset ()
  {
    SendRequest (new RevokeRequest (_session));
    Advertiser::Reset ();
  }

  // --------------------------------------------------------------------------------
  gboolean Facebook::IsOopCapable ()
  {
    return FALSE;
  }

#ifdef WEBKIT
  // --------------------------------------------------------------------------------
  gboolean Facebook::OnRedirect (WebKitNetworkRequest    *request,
                                 WebKitWebPolicyDecision *policy_decision)
  {
    const gchar *request_uri = webkit_network_request_get_uri (request);

    if (   g_strrstr (request_uri, "https://www.facebook.com/connect/login_success.html#")
        || g_strrstr (request_uri, "https://www.facebook.com/connect/login_success.html?"))
    {
      {
        gchar **params = g_strsplit_set (request_uri,
                                         "?#&",
                                         -1);

        for (guint p = 0; params[p] != NULL; p++)
        {
          if (g_strrstr (params[p], "access_token="))
          {
            gchar **code = g_strsplit_set (params[p],
                                           "=",
                                           -1);

            if (code[0] && code[1])
            {
              _session->SetToken (code[1]);
            }
            g_strfreev (code);
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
  gboolean Facebook::HandleRequestResponse (Oauth::Request *request)
  {
    if (dynamic_cast <DebugTokenRequest *> (request))
    {
      SendRequest (new MeRequest (_session));
      return TRUE;
    }

    return FALSE;
  }
}
