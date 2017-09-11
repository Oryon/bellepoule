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

#include "twitter.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  Twitter::Twitter ()
    : Object ("Twitter"),
      Advertiser ("twitter")
  {
    Oauth::Session *session = new Oauth::Session ("E7YgKcY2Yt9bHLxceaVBSg",
                                                  "8HnMWMXOZgCrRE5VFILIlx0pQUuXIxkgd5aYh34rfg");

    session->SetToken       ("905893129982705673-jrcZ4jJGoMRaO8WMlSxjHMAJfkz9340");
    session->SetTokenSecret ("8lRYb357PLZXgPLVwktoOUDZsM7JfRXJ3rj2OvhhQR2mf");

    SetSession (session);
  }

  // --------------------------------------------------------------------------------
  Twitter::~Twitter ()
  {
  }
}
