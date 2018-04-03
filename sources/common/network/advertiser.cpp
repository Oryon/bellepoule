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

#ifdef WEBKIT
#include <webkit/webkit.h>
#endif

#include "util/global.hpp"
#include "util/glade.hpp"

#include "oauth/v1_request.hpp"
#include "oauth/v1_access_token.hpp"
#include "oauth/session.hpp"

#include "advertiser.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  Advertiser::Advertiser (const gchar *name)
    : Object ("Advertiser"),
      Module ("advertiser.glade")
  {
    _state            = WARMUP;
    _title            = NULL;
    _link             = NULL;
    _session          = NULL;
    _pending_response = FALSE;
#ifdef WEBKIT
    _oob_authentication = FALSE;
#else
    _oob_authentication = TRUE;
#endif

    {
      gchar *png  = g_strdup_printf ("%s.png", name);
      gchar *path = g_build_filename (Global::_share_dir, "resources", "glade", "images", png, NULL);

      gtk_image_set_from_file (GTK_IMAGE (_glade->GetWidget ("dialog_image")),
                               path);
      gtk_image_set_from_file (GTK_IMAGE (_glade->GetWidget ("logo")),
                               path);

      g_free (path);
      g_free (png);
    }

    {
      GtkMessageDialog *dialog = GTK_MESSAGE_DIALOG (_glade->GetWidget ("request_token_dialog"));

      gtk_message_dialog_format_secondary_markup (dialog,
                                                  "%s <b>%s</b> :",
                                                  gettext ("Follow the instructions given by "),
                                                  name);
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
  gboolean Advertiser::IsOopCapable ()
  {
    return TRUE;
  }

  // --------------------------------------------------------------------------------
  void Advertiser::Plug (GtkTable *in)
  {
    GtkWidget *header = _glade->GetWidget ("header");
    GtkWidget *entry  = _glade->GetWidget ("entry");
    guint      n_rows = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (in),
                                                        "n-advertisers"));

    gtk_table_attach_defaults (in,
                               header,
                               0, 1,
                               n_rows, n_rows+1);
    gtk_table_attach_defaults (in,
                               entry,
                               1, 2,
                               n_rows, n_rows+1);

#ifndef WEBKIT
    if (IsOopCapable () == FALSE)
    {
      gtk_widget_set_sensitive (header, FALSE);
      gtk_widget_set_sensitive (entry,  FALSE);
    }
#endif

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

    CheckAuthorization ();
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

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (togglebutton),
                                  (_state == ON));
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
  void Advertiser::SendRequest (Oauth::Request *request)
  {
    Oauth::Uploader *uploader = new Oauth::Uploader (this);

    _pending_response = TRUE;
    uploader->UpLoadRequest (request);

    request->Release ();
    uploader->Release ();
  }

  // --------------------------------------------------------------------------------
  void Advertiser::SwitchOn ()
  {
    CheckAuthorization ();
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
  void Advertiser::Publish (const gchar *event)
  {
    if ((_state == ON) && _title)
    {
      gchar *message;

      if (_link)
      {
        message = g_strdup_printf ("%s\n%s\n%s", _title, event, _link);
      }
      else
      {
        message = g_strdup_printf ("%s\n%s", _title, event);
      }

      PublishMessage (message);
      g_free (message);
    }
  }

  // --------------------------------------------------------------------------------
  void Advertiser::TweetFeeder (Advertiser *advertiser,
                                Feeder     *feeder)
  {
    gchar *event = feeder->GetAnnounce ();

    advertiser->Publish (event);

    g_free (event);
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

  // --------------------------------------------------------------------------------
  void Advertiser::DisplayAuthorizationPage ()
  {
#ifdef WEBKIT
    GtkWidget *web_view = webkit_web_view_new ();

    {
      gtk_container_add (GTK_CONTAINER (_glade->GetWidget ("webview")),
                         web_view);
      g_signal_connect (G_OBJECT (web_view),
                        "navigation-policy-decision-requested",
                        G_CALLBACK (OnWebKitRedirect),
                        this);

      webkit_web_view_load_uri (WEBKIT_WEB_VIEW (web_view),
                                _session->GetAuthorizationPage ());

      gtk_widget_show (web_view);
      gtk_widget_grab_focus (web_view);
    }

    {
      GtkWidget *dialog = _glade->GetWidget ("webkit_dialog");

      RunDialog (GTK_DIALOG (dialog));
      gtk_widget_hide (dialog);
    }

    gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (web_view)),
                          web_view);

    if (_pending_response == FALSE)
    {
      CheckAuthorization ();
    }
#else
    {
      GtkWidget *link = _glade->GetWidget ("pin_url");

      gtk_link_button_set_uri (GTK_LINK_BUTTON (link),
                               _session->GetAuthorizationPage ());
    }

    {
      GtkWidget *dialog = _glade->GetWidget ("request_token_dialog");
      GtkEntry  *entry  = GTK_ENTRY (_glade->GetWidget ("pin_entry"));

      if (entry)
      {
        gtk_entry_set_text (entry,
                            "");
      }

      if (RunDialog (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
      {
        SendRequest (new Oauth::V1::AccessToken (_session,
                                                 gtk_entry_get_text (entry)));
      }
      else
      {
        DisplayId ("");
      }

      gtk_widget_hide (dialog);
    }
#endif
  }

  // --------------------------------------------------------------------------------
  gboolean Advertiser::OnWebKitRedirect (WebKitWebView             *web_view,
                                         WebKitWebFrame            *frame,
                                         WebKitNetworkRequest      *request,
                                         WebKitWebNavigationAction *navigation_action,
                                         WebKitWebPolicyDecision   *policy_decision,
                                         Advertiser                *a)
  {
    return a->OnRedirect (request,
                          policy_decision);
  }

  // --------------------------------------------------------------------------------
  void Advertiser::PublishMessage (const gchar *message)
  {
  }

  // --------------------------------------------------------------------------------
  void Advertiser::OnServerResponse (Oauth::Request *request)
  {
    _pending_response = FALSE;

    if (request->GetStatus () == Oauth::Request::NETWORK_ERROR)
    {
      _state = OFF;
      DisplayId ("Network error!");
    }
    else if (request->GetStatus () == Oauth::Request::REJECTED)
    {
      if (_state == WARMUP)
      {
        _state = OFF;
      }
      else if (_state == OFF)
      {
        _state = WAITING_FOR_TOKEN;
        _session->Reset ();
        ClaimForAuthorization ();
      }
      else
      {
        _state = OFF;
        DisplayId ("Access denied!");
      }
    }
    else if (HandleRequestResponse (request) == FALSE)
    {
      if (_state == WARMUP)
      {
        _state = OFF;
      }
      else
      {
        _state = ON;
      }
      DisplayId (_session->GetAccountId ());
    }
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
