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

#pragma once

#include <webkit/webkit.h>

#include "util/module.hpp"
#include "oauth/uploader.hpp"

namespace Oauth
{
  class Session;
  class Request;
}

namespace Net
{
  class Advertiser : public Module, public Oauth::Uploader::Listener
  {
    public:
      class Feeder
      {
        public:
          virtual gchar *GetAnnounce () = 0;
      };

    public:
      Advertiser (const gchar *name);

      void Plug (GtkTable *in);

      void OnResetAccount ();

      void OnToggled (gboolean on);

      void Publish (const gchar *event);

      static void SetTitle (Advertiser  *advertiser,
                            const gchar *title);

      static void SetLink (Advertiser  *advertiser,
                           const gchar *link);

      static void TweetFeeder (Advertiser *advertiser,
                               Feeder     *feeder);

    protected:
      Oauth::Session *_session;
      gchar          *_name;

      ~Advertiser ();

      void SetSession (Oauth::Session *session);

      void SendRequest (Oauth::Request *request);

    protected:
      void Reset ();

      void DisplayId (const gchar *id);

      void DisplayAuthorizationPage ();

    private:
      enum State
      {
        WARMUP,
        OFF,
        WAITING_FOR_TOKEN,
        ON
      };

      gchar    *_link;
      gchar    *_title;
      gboolean  _pending_response;
      State     _state;

      void SwitchOn ();

      void SwitchOff ();

      void Use ();

      void Drop ();

      void OnServerResponse (Oauth::Request *request);

      virtual void PublishMessage (const gchar *message) = 0;

      virtual void CheckAuthorization () = 0;

      virtual void ClaimForAuthorization () = 0;

      virtual gboolean HandleRequestResponse (Oauth::Request *request) = 0;

      virtual gboolean OnRedirect (WebKitNetworkRequest    *request,
                                   WebKitWebPolicyDecision *policy_decision) = 0;

      static gboolean OnWebKitRedirect (WebKitWebView             *web_view,
                                        WebKitWebFrame            *frame,
                                        WebKitNetworkRequest      *request,
                                        WebKitWebNavigationAction *navigation_action,
                                        WebKitWebPolicyDecision   *policy_decision,
                                        Advertiser                *t);
  };
}
