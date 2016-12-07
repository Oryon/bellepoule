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

#include "util/object.hpp"

namespace Oauth
{
  class Session;

  class HttpRequest : public Object
  {
    public:
      HttpRequest (Session     *session,
                   const gchar *http_method,
                   const gchar *class_name);

      virtual void ParseResponse (const gchar *response);

      void AddHeaderField (const gchar *key,
                                        const gchar *value);

      gchar *GetHeader ();

      virtual const gchar *GetURL () = 0;

    protected:
      static const gchar *GET;
      Session            *_session;

      ~HttpRequest ();

      gchar *ExtractParsedField (const gchar *field_desc,
                                 const gchar *field_name);

    private:
      static const gchar  _nonce_range[];
      GRand              *_rand;
      GList              *_header_list;
      const gchar        *_http_method;

      void Stamp ();

      char *GetNonce ();

      void Sign ();
  };
}
