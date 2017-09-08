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

#include "oauth/http_request.hpp"
#include "oauth/session.hpp"
#include "oauth/request_token.hpp"
#include "oauth/access_token.hpp"

#include "verify_credentials_request.hpp"
#include "statuses_update_request.hpp"

#include "twitter.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  Twitter::Twitter (Listener *listener)
    : Object ("Twitter"),
      Module ("twitter.glade")
  {
    _listener = listener;

    _state = OFF;

    _session = new Oauth::Session ("E7YgKcY2Yt9bHLxceaVBSg",
                                   "8HnMWMXOZgCrRE5VFILIlx0pQUuXIxkgd5aYh34rfg");

    _session->SetToken       ("905893129982705673-jrcZ4jJGoMRaO8WMlSxjHMAJfkz9340");
    _session->SetTokenSecret ("8lRYb357PLZXgPLVwktoOUDZsM7JfRXJ3rj2OvhhQR2mf");

    _listener->OnTwitterID ("");
  }

  // --------------------------------------------------------------------------------
  Twitter::~Twitter ()
  {
    _session->Release ();
  }

  // --------------------------------------------------------------------------------
  void Twitter::OnTwitterResponse (Oauth::HttpRequest *request)
  {
    VerifyCredentials *verify_credentials = dynamic_cast <VerifyCredentials *> (request);

    if (request->GetStatus () == Oauth::HttpRequest::NETWORK_ERROR)
    {
      _state = OFF;
      _listener->OnTwitterID ("Network error!");
    }
    else if (request->GetStatus () == Oauth::HttpRequest::REJECTED)
    {
      if ((_state == OFF) && verify_credentials)
      {
        _state = WAITING_FOR_TOKEN;
        _session->Reset ();
        SendRequest (new Oauth::RequestToken (_session));
      }
      else
      {
        _state = OFF;
        _listener->OnTwitterID ("Access denied!");
      }
    }
    else
    {
      Oauth::RequestToken *request_token = dynamic_cast <Oauth::RequestToken *> (request);

      if (request_token)
      {
        gchar *pin_url = request_token->GetPinCodeUrl ();

        {
          GtkWidget *link = _glade->GetWidget ("pin_url");

          gtk_link_button_set_uri (GTK_LINK_BUTTON (link),
                                   pin_url);

          g_free (pin_url);
        }

        {
          GtkWidget *dialog = _glade->GetWidget ("request_token_dialog");
          GtkEntry  *entry  = GTK_ENTRY (_glade->GetWidget ("pin_entry"));

          if (entry)
          {
            gtk_entry_set_text (entry,
                                "");
          }

          switch (RunDialog (GTK_DIALOG (dialog)))
          {
            case GTK_RESPONSE_OK:
            {
              SendRequest (new Oauth::AccessToken (_session,
                                                   gtk_entry_get_text (entry)));

            }
            break;

            default:
            {
              _listener->OnTwitterID ("");
            }
            break;
          }

          gtk_widget_hide (dialog);
        }
      }
      else if (dynamic_cast <Oauth::AccessToken *> (request))
      {
        SendRequest (new VerifyCredentials (_session));
      }
      else if (verify_credentials)
      {
        _state = ON;
        _listener->OnTwitterID (verify_credentials->_twitter_account);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Twitter::Use ()
  {
    Retain ();
  }

  // --------------------------------------------------------------------------------
  void Twitter::Drop ()
  {
    Release ();
  }

  // --------------------------------------------------------------------------------
  void Twitter::SendRequest (Oauth::HttpRequest *request)
  {
    TwitterUploader *uploader = new TwitterUploader (this);

    uploader->UpLoadRequest (request);

    request->Release ();
    uploader->Release ();
  }

  // --------------------------------------------------------------------------------
  void Twitter::SwitchOn ()
  {
    SendRequest (new VerifyCredentials (_session));
  }

  // --------------------------------------------------------------------------------
  void Twitter::SwitchOff ()
  {
    _state = OFF;
  }

  // --------------------------------------------------------------------------------
  void Twitter::Reset ()
  {
    _session->Reset ();
  }

  // --------------------------------------------------------------------------------
  void Twitter::Tweet (const gchar *tweet)
  {
    if (_state == ON)
    {
      SendRequest (new StatusesUpdate (_session,
                                       tweet));
    }
  }

  // --------------------------------------------------------------------------------
  void Twitter::Tweet (Feeder *feeder)
  {
    if (_state == ON)
    {
      SendRequest (new StatusesUpdate (_session,
                                       feeder->GetTweet ()));
    }
  }
}
