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

namespace Oauth
{
  class Session : public virtual Object
  {
    public:
      Session (const gchar *service,
               const gchar *api_uri,
               const gchar *consumer_key,
               const gchar *consumer_secret);

      virtual void Reset ();

      void SetToken (const gchar *token);

      const gchar *GetService ();

      const gchar *GetConsumerKey ();

      const gchar *GetConsumerSecret ();

      const gchar *GetApiUri ();

      const gchar *GetToken ();

      void SetAccountId (const gchar *id);

      const gchar *GetAccountId ();

      void SetAuthorizationPage (const gchar *id);

      const gchar *GetAuthorizationPage ();

    protected:
      ~Session () override;

    private:
      gchar *_service;
      gchar *_consumer_key;
      gchar *_consumer_secret;
      gchar *_token;
      gchar *_api_uri;
      gchar *_account_id;
      gchar *_authorization_page;
  };
}
