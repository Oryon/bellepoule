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

#include "twitter_uploader.hpp"

namespace Oauth
{
  class Session;
}

namespace Net
{
  class Twitter : public Module, public TwitterUploader::Listener
  {
    public:
      class Listener
      {
        public:
          virtual void OnTwitterID (const gchar *id) = 0;
      };

    public:
      Twitter (Listener *listener);

      void SwitchOn ();

      void SwitchOff ();

      void Reset ();

    private:
      enum State
      {
        OFF,
        WAITING_FOR_TOKEN,
        ON
      };

      Listener       *_listener;
      Oauth::Session *_session;
      State           _state;

      ~Twitter ();

      void SendRequest (Oauth::HttpRequest *request);

      void OnTwitterResponse (Oauth::HttpRequest *request);

      void Use ();

      void Drop ();
  };
}
