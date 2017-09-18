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

#include "oauth/http_request.hpp"
#include "oauth/session.hpp"
#include "oauth/request_token.hpp"
#include "oauth/access_token.hpp"

#include "twitter/verify_credentials_request.hpp"
#include "twitter/statuses_update_request.hpp"

#include "advertiser.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  Advertiser::Advertiser (const gchar *name)
    : Object ("Advertiser"),
      Module ("advertiser.glade")
  {
    _state           = IDLE;
    _title           = NULL;
    _link            = NULL;
    _session         = NULL;
    _pending_request = FALSE;

    {

      gchar    *png  = g_strdup_printf ("%s.png", name);
      GtkImage *logo = GTK_IMAGE (_glade->GetWidget ("logo"));
      gchar    *path = g_build_filename (Global::_share_dir, "resources", "glade", "images", png, NULL);

      gtk_image_set_from_file (logo,
                               path);
      g_free (path);
      g_free (png);
    }

    {
      GdkDisplay *display     = gdk_display_get_default ();
      GdkScreen  *screen      = gdk_display_get_default_screen (display);
      gint        full_height = gdk_screen_get_height (screen);

      gtk_widget_set_size_request (_glade->GetWidget ("webview"),
                                   700,
                                   (full_height*70)/100);
    }

    gtk_button_set_label (GTK_BUTTON (_glade->GetWidget ("checkbutton")),
                          name);

    _name = g_strdup (name);

    DisplayId ("");
  }

  // --------------------------------------------------------------------------------
  Advertiser::~Advertiser ()
  {
    Object::TryToRelease (_session);
    g_free (_title);
    g_free (_name);
  }

  // --------------------------------------------------------------------------------
  void Advertiser::Plug (GtkTable *in)
  {
    guint n_rows = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (in),
                                                        "n-advertisers"));

    gtk_table_attach_defaults (in,
                               _glade->GetWidget ("header"),
                               0, 1,
                               n_rows, n_rows+1);
    gtk_table_attach_defaults (in,
                               _glade->GetWidget ("entry"),
                               1, 2,
                               n_rows, n_rows+1);

    g_object_set_data (G_OBJECT (in),
                       "n-advertisers",
                       GUINT_TO_POINTER (n_rows+1));
  }

  // --------------------------------------------------------------------------------
  void Advertiser::SetSession (Oauth::Session *session)
  {
    _session = session;

    gtk_widget_set_sensitive (_glade->GetWidget ("header"),
                              TRUE);
    gtk_widget_set_sensitive (_glade->GetWidget ("entry"),
                              TRUE);

    SendRequest (new VerifyCredentials (_session));
  }

  // --------------------------------------------------------------------------------
  void Advertiser::DisplayId (const gchar *id)
  {
    GtkWidget *togglebutton = _glade->GetWidget ("checkbutton");
    GtkWidget *spinner      = _glade->GetWidget ("spinner");
    GtkEntry  *entry        = GTK_ENTRY (_glade->GetWidget ("entry"));

    gtk_widget_hide (spinner);
    gtk_widget_set_sensitive (togglebutton,
                              TRUE);

    gtk_entry_set_text (entry,
                        id);

    if (g_strstr_len (id,
                      -1,
                      "@") == NULL)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (togglebutton),
                                    FALSE);
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Advertiser::OnRedirect (WebKitWebView             *web_view,
                                   WebKitWebFrame            *frame,
                                   WebKitNetworkRequest      *request,
                                   WebKitWebNavigationAction *navigation_action,
                                   WebKitWebPolicyDecision   *policy_decision,
                                   Advertiser                *a)
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
              a->SendRequest (new Oauth::AccessToken (a->_session,
                                                      verifyer[1]));
            }
            g_strfreev (verifyer);
            break;
          }
        }

        g_strfreev (params);
      }

      gtk_dialog_response (GTK_DIALOG (a->_glade->GetWidget ("request_token_dialog")),
                           GTK_RESPONSE_CANCEL);

      webkit_web_policy_decision_ignore (policy_decision);
      return TRUE;
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void Advertiser::OnTwitterResponse (Oauth::HttpRequest *request)
  {
    VerifyCredentials *verify_credentials = dynamic_cast <VerifyCredentials *> (request);

    _pending_request = FALSE;

    if (request->GetStatus () == Oauth::HttpRequest::NETWORK_ERROR)
    {
      _state = OFF;
      DisplayId ("Network error!");
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
        if (_state != IDLE)
        {
          DisplayId ("Access denied!");
        }
        _state = OFF;
      }
    }
    else
    {
      Oauth::RequestToken *request_token = dynamic_cast <Oauth::RequestToken *> (request);

      if (request_token)
      {
        GtkWidget *web_view = webkit_web_view_new ();

        {
          gchar *pin_url = request_token->GetPinCodeUrl ();

          gtk_container_add (GTK_CONTAINER (_glade->GetWidget ("webview")),
                             web_view);
          g_signal_connect (G_OBJECT (web_view),
                            "navigation-policy-decision-requested",
                            G_CALLBACK (OnRedirect),
                            this);

          webkit_web_view_load_uri (WEBKIT_WEB_VIEW (web_view),
                                    pin_url);

          gtk_widget_show (web_view);
          gtk_widget_grab_focus (web_view);

          g_free (pin_url);
        }

        {
          GtkWidget *dialog = _glade->GetWidget ("request_token_dialog");

          RunDialog (GTK_DIALOG (dialog));
          gtk_widget_hide (dialog);

          if (_pending_request == FALSE)
          {
            SendRequest (new VerifyCredentials (_session));
          }
        }

        gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (web_view)),
                              web_view);
      }
      else if (dynamic_cast <Oauth::AccessToken *> (request))
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
  void Advertiser::Use ()
  {
    Retain ();
  }

  // --------------------------------------------------------------------------------
  void Advertiser::Drop ()
  {
    Release ();
  }

  // --------------------------------------------------------------------------------
  void Advertiser::SendRequest (Oauth::HttpRequest *request)
  {
    TwitterUploader *uploader = new TwitterUploader (this);

    _pending_request = TRUE;
    uploader->UpLoadRequest (request);

    request->Release ();
    uploader->Release ();
  }

  // --------------------------------------------------------------------------------
  void Advertiser::SwitchOn ()
  {
    SendRequest (new VerifyCredentials (_session));
  }

  // --------------------------------------------------------------------------------
  void Advertiser::SwitchOff ()
  {
    _state = OFF;
  }

  // --------------------------------------------------------------------------------
  void Advertiser::Reset ()
  {
    _session->Reset ();
  }

  // --------------------------------------------------------------------------------
  void Advertiser::SetTitle (Advertiser  *advertiser,
                             const gchar *title)
  {
    g_free (advertiser->_title);
    advertiser->_title = NULL;

    if (title)
    {
      advertiser->_title = g_strdup_printf ("\xf0\x9f\xa4\xba %s", title);
    }
  }

  // --------------------------------------------------------------------------------
  void Advertiser::SetLink (Advertiser  *advertiser,
                            const gchar *link)
  {
    g_free (advertiser->_link);
    advertiser->_link = NULL;

    if (link)
    {
      advertiser->_link = g_strdup (link);
    }
  }

  // --------------------------------------------------------------------------------
  void Advertiser::Tweet (const gchar *tweet)
  {
    if ((_state == ON) && _title)
    {
      gchar *message;

      if (_link)
      {
        message = g_strdup_printf ("%s\n%s\n%s", _title, tweet, _link);
      }
      else
      {
        message = g_strdup_printf ("%s\n%s", _title, tweet);
      }

      SendRequest (new StatusesUpdate (_session,
                                       message));
      g_free (message);
    }
  }

  // --------------------------------------------------------------------------------
  void Advertiser::TweetFeeder (Advertiser *advertiser,
                                Feeder     *feeder)
  {
    gchar *tweet = feeder->GetTweet ();

    advertiser->Tweet (tweet);

    g_free (tweet);
  }

  // --------------------------------------------------------------------------------
  void Advertiser::OnToggled (gboolean on)
  {
    if (on)
    {
      GtkWidget *togglebutton = _glade->GetWidget ("checkbutton");
      GtkWidget *spinner      = _glade->GetWidget ("spinner");
      GtkEntry  *entry        = GTK_ENTRY (_glade->GetWidget ("entry"));

      gtk_widget_show (spinner);
      gtk_widget_set_sensitive (togglebutton,
                                FALSE);

      gtk_entry_set_text (entry,
                          "");

      SwitchOn ();
    }
    else
    {
      SwitchOff ();
    }
  }

  // --------------------------------------------------------------------------------
  void Advertiser::OnResetAccount ()
  {
    GtkEntry        *entry        = GTK_ENTRY         (_glade->GetWidget ("entry"));
    GtkToggleButton *togglebutton = GTK_TOGGLE_BUTTON (_glade->GetWidget ("checkbutton"));

    gtk_toggle_button_set_active (togglebutton,
                                  FALSE);

    gtk_entry_set_text (entry,
                        "");

    Reset ();
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_checkbutton_toggled (GtkToggleButton *button,
                                                        Object          *owner)
{
  Net::Advertiser *a = dynamic_cast <Net::Advertiser *> (owner);

  a->OnToggled (gtk_toggle_button_get_active (button));
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_entry_icon_press (GtkEntry             *entry,
                                                     GtkEntryIconPosition  icon_pos,
                                                     GdkEvent             *event,
                                                     Object               *owner)
{
  Net::Advertiser *a = dynamic_cast <Net::Advertiser *> (owner);

  a->OnResetAccount ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_pin_url_clicked (GtkWidget *widget,
                                                    Object    *owner)
{
  const gchar *uri = gtk_link_button_get_uri (GTK_LINK_BUTTON (widget));

#ifdef WINDOWS_TEMPORARY_PATCH
  ShellExecute (NULL,
                "open",
                uri,
                NULL,
                NULL,
                SW_SHOWNORMAL);
#else
  gtk_show_uri (NULL,
                uri,
                GDK_CURRENT_TIME,
                NULL);
#endif
}

