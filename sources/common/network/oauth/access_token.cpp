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

#include "access_token.hpp"

namespace Oauth
{
  // --------------------------------------------------------------------------------
  AccessToken::AccessToken (Session     *session,
                            const gchar *pin)
    : HttpRequest (session, "GET", "Oauth::AccessToken")
  {
    AddHeaderField ("oauth_verifier", pin);
  }

  // --------------------------------------------------------------------------------
  AccessToken::~AccessToken ()
  {
  }

  // --------------------------------------------------------------------------------
  const gchar *AccessToken::GetURL ()
  {
    return "https://api.twitter.com/oauth/access_token";
  }
}
