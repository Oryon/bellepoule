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
    _state   = OFF;
    _title   = NULL;
    _link    = NULL;
    _session = NULL;

    {

      gchar    *png  = g_strdup_printf ("%s.png", name);
      GtkImage *logo = GTK_IMAGE (_glade->GetWidget ("logo"));
      gchar    *path = g_build_filename (Global::_share_dir, "resources", "glade", "images", png, NULL);

      gtk_image_set_from_file (logo,
                               path);
      g_free (path);
      g_free (png);
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
  void Advertiser::OnTwitterResponse (Oauth::HttpRequest *request)
  {
    VerifyCredentials *verify_credentials = dynamic_cast <VerifyCredentials *> (request);

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
        _state = OFF;
        DisplayId ("Access denied!");
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
              DisplayId ("");
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
      advertiser->_title = g_strdup_printf ("%c%c%c%c %s", 0xF0, 0x9F, 0xA4, 0xBA, title);
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
    if ((_state == ON) && (_title))
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
