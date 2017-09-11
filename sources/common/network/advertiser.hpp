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

#include "util/module.hpp"

#include "twitter/twitter_uploader.hpp"

namespace Oauth
{
  class Session;
}

namespace Net
{
  class Advertiser : public Module, public TwitterUploader::Listener
  {
    public:
      class Feeder
      {
        public:
          virtual gchar *GetTweet () = 0;
      };

    public:
      Advertiser (const gchar *name);

      void SetTitle (const gchar *title);

      void SetLink (const gchar *link);

      void Tweet (const gchar *tweet);

      void Tweet (Feeder *feeder);

      void Plug (GtkTable *in);

    private:
      void SwitchOn ();

      void SwitchOff ();

      void Reset ();

    public:
      void OnResetAccount ();

      void OnToggled (gboolean on);

    protected:
      Oauth::Session *_session;

      ~Advertiser ();

      void SetSession (Oauth::Session *session);

    private:
      enum State
      {
        OFF,
        WAITING_FOR_TOKEN,
        ON
      };

      State  _state;
      gchar *_name;
      gchar *_link;
      gchar *_title;

      void SendRequest (Oauth::HttpRequest *request);

      void OnTwitterResponse (Oauth::HttpRequest *request);

      void DisplayId (const gchar *id);

      void Use ();

      void Drop ();
  };
}
