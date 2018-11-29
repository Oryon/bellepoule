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

#include "advertiser.hpp"

namespace Oauth
{
  class Request;
}

namespace Net
{
  class Twitter : public Advertiser
  {
    public:
      Twitter ();

    private:
      ~Twitter () override;

      void PublishMessage (const gchar *message) override;

      gboolean HandleRequestResponse (Oauth::Request *request) override;

      void CheckAuthorization () override;

      void ClaimForAuthorization () override;

#ifdef WEBKIT
      gboolean OnRedirect (WebKitNetworkRequest    *request,
                           WebKitWebPolicyDecision *policy_decision) override;
#endif
  };
}
