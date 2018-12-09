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
#include "me_request.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  MeRequest::MeRequest (Oauth::Session *session)
    : Object ("Facebook::MeRequest"),
    Oauth::V2::Request (session, "user_id", HTTP_GET)
  {
    Session *facebook_session = dynamic_cast <Session *> (_session);

    SetSignature (facebook_session->GetUserId ());
  }

  // --------------------------------------------------------------------------------
  MeRequest::~MeRequest ()
  {
  }

  // --------------------------------------------------------------------------------
  void MeRequest::ParseResponse (GHashTable  *header,
                                 const gchar *body)
  {
    Oauth::Request::ParseResponse (header,
                                   body);

    if (GetStatus () == Status::ACCEPTED)
    {
      if (LoadJson (body))
      {
        gchar *id = GetJsonAtPath ("$.name");

        _session->SetAccountId (id);
        g_free (id);
      }
    }
  }
}
