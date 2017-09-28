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

#include "oauth/session.hpp"
#include "session.hpp"
#include "revoke_request.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  RevokeRequest::RevokeRequest (Oauth::Session *session)
    : Object ("Facebook::RevokeRequest"),
    Oauth::V2::Request (session, "user_id", HTTP_DELETE)
  {
    Session *facebook_session = dynamic_cast <Session *> (_session);
    gchar   *signature = g_strdup_printf ("%s/permissions/publish_actions",
                                          facebook_session->GetUserId ());

    SetSignature (signature);
    g_free (signature);
  }

  // --------------------------------------------------------------------------------
  RevokeRequest::~RevokeRequest ()
  {
  }
}
