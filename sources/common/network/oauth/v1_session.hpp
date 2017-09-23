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

#include <glib.h>

#include "util/object.hpp"
#include "session.hpp"

namespace Oauth
{
  namespace V1
  {
    class Session : public Oauth::Session
    {
      public:
        Session (const gchar *service,
                 const gchar *api_uri,
                 const gchar *consumer_key,
                 const gchar *consumer_secret);

        void Reset ();

      public:
        void SetTokenSecret (const gchar *token_secret);

        const guchar *GetSigningKey ();

      private:
        GString *_signing_key;

        virtual ~Session ();
    };
  }
}
