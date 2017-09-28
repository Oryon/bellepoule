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

#include <stdio.h>
#include <string.h>

#include "v1_session.hpp"
#include "v1_request_token.hpp"

namespace Oauth
{
  namespace V1
  {
    // --------------------------------------------------------------------------------
    RequestToken::RequestToken (Oauth::Session *session)
      : Object ("Oauth::V1::RequestToken"),
        Request (session, "oauth/request_token", HTTP_GET)
    {
      //AddHeaderField ("oauth_callback", "oob");
      AddHeaderField ("oauth_callback", "http://bellepoule.bzh");
    }

    // --------------------------------------------------------------------------------
    RequestToken::~RequestToken ()
    {
    }

    // --------------------------------------------------------------------------------
    gchar *RequestToken::GetPinCodeUrl ()
    {
      return g_strdup_printf ("%s/oauth/authorize?oauth_token=%s",
                              _session->GetApiUri (),
                              _session->GetToken ());
    }
  }
}
