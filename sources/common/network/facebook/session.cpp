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

#include "session.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  Session::Session (const gchar *service,
                    const gchar *api_uri,
                    const gchar *consumer_key,
                    const gchar *consumer_secret)
    : Object ("Facebook::Session"),
      Oauth::Session (service,
                      api_uri,
                      consumer_key,
                      consumer_secret)
  {
    _user_id = nullptr;

  }

  // --------------------------------------------------------------------------------
  Session::~Session ()
  {
    g_free (_user_id);
  }

  // --------------------------------------------------------------------------------
  void Session::SetUserId (const gchar *id)
  {
    g_free (_user_id);
    _user_id = g_strdup (id);
  }

  // --------------------------------------------------------------------------------
  const gchar *Session::GetUserId ()
  {
    return _user_id;
  }
}
