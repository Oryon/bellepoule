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

#include "request.hpp"

namespace Oauth
{
  class Session;

  namespace V1
  {
    class Session;

    class Request : public Oauth::Request
    {
      public:
        Request (Oauth::Session *session,
                 const gchar    *sub_url,
                 const gchar    *http_method);

        virtual void ParseResponse (GHashTable  *header,
                                    const gchar *body);

        virtual struct curl_slist *GetHeader ();

      protected:
        ~Request ();

        gchar *ExtractParsedField (const gchar *field_desc,
                                   const gchar *field_name);

      private:
        static const gchar  _nonce_range[];
        GRand              *_rand;

        void Stamp ();

        char *GetNonce ();

        void Sign ();

        void DumpRateLimits (GHashTable  *header);

        Session *GetSession ();
    };
  }
}
