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

#include <json-glib/json-glib.h>

#include "util/object.hpp"

namespace Oauth
{
  class Session;
  namespace V1
  {
    class Session;
  }

  class HttpRequest : public virtual Object
  {
    public:
      enum Status
      {
        READY,
        NETWORK_ERROR,
        REJECTED,
        ACCEPTED
      };

    public:
      HttpRequest (Session     *session,
                   const gchar *sub_url,
                   const gchar *http_method);

      virtual void ParseResponse (GHashTable  *header,
                                  const gchar *body);

      void AddHeaderField (const gchar *key,
                           const gchar *value);

      void AddParameterField (const gchar *key,
                              const gchar *value);

      Status GetStatus ();

      gchar *GetHeader ();

      const gchar *GetURL ();

      gchar *GetParameters ();

      const gchar *GetMethod ();

    protected:
      static const gchar *GET;
      static const gchar *POST;
      V1::Session        *_session;
      gchar              *_url;
      JsonParser         *_parser;

      ~HttpRequest ();

      gchar *ExtractParsedField (const gchar *field_desc,
                                 const gchar *field_name);

      gboolean LoadJson (const gchar *json);

      gchar *GetJsonAtPath (const gchar *path);

      void ForgiveError ();

    private:
      static const gchar  _nonce_range[];
      GRand              *_rand;
      GList              *_header_list;
      GList              *_parameter_list;
      const gchar        *_http_method;
      Status              _status;

      void Stamp ();

      char *GetNonce ();

      void Sign ();

      void DumpRateLimits (GHashTable  *header);
  };
}
