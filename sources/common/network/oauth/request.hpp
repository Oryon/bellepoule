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

struct curl_slist;

namespace Oauth
{
  class Session;

  class Request : public virtual Object
  {
      public:
        enum class Status
        {
          READY,
          NETWORK_ERROR,
          REJECTED,
          ACCEPTED
        };

    public:
      Request (Session     *session,
               const gchar *signature,
               const gchar *http_method);

      virtual void ParseResponse (GHashTable  *header,
                                  const gchar *body);

      virtual struct curl_slist *GetHeader ();

      const gchar *GetMethod ();

      Status GetStatus ();

      gchar *GetParameters ();

      const gchar *GetURL ();

      void ForgiveError ();

      void SetSignature (const gchar *signature);

    protected:
      static const gchar *HTTP_GET;
      static const gchar *HTTP_POST;
      static const gchar *HTTP_DELETE;

      Status   _status;
      gchar   *_url;
      Session *_session;
      GList   *_header_list;
      GList   *_parameter_list;

      ~Request () override;

      gboolean LoadJson (const gchar *json);

      gchar *GetJsonAtPath (const gchar *path);

      void AddHeaderField (const gchar *key,
                           const gchar *value);

      void AddParameterField (const gchar *key,
                              const gchar *value);

    private:
      const gchar *_http_method;
      JsonParser  *_parser;
  };
}
