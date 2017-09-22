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
#include "request.hpp"

namespace Oauth
{
  const gchar *Request::GET  = "GET";
  const gchar *Request::POST = "POST";

  // --------------------------------------------------------------------------------
  Request::Request (Session     *session,
                    const gchar *sub_url,
                    const gchar *http_method)
    : Object ("Oauth::Request")
  {
    _session     = session;
    _status      = READY;
    _http_method = http_method;
    _url         = g_strdup_printf ("%s/%s", session->GetApiUri (), sub_url);
  }

  // --------------------------------------------------------------------------------
  Request::~Request ()
  {
    g_free (_url);
  }

  // --------------------------------------------------------------------------------
  const gchar *Request::GetMethod ()
  {
    return _http_method;
  }

  // --------------------------------------------------------------------------------
  const gchar *Request::GetURL ()
  {
    return _url;
  }

  // --------------------------------------------------------------------------------
  void Request::ForgiveError ()
  {
    _status = ACCEPTED;
  }

  // --------------------------------------------------------------------------------
  Request::Status Request::GetStatus ()
  {
    return _status;
  }

  // --------------------------------------------------------------------------------
  gboolean Request::LoadJson (const gchar *json)
  {
    if (_parser == NULL)
    {
      _parser = json_parser_new ();
    }

    return json_parser_load_from_data (_parser,
                                       json,
                                       -1,
                                       NULL);
  }

  // --------------------------------------------------------------------------------
  gchar *Request::GetJsonAtPath (const gchar *path)
  {
    gchar *result = NULL;

    if (_parser)
    {
      JsonNode *node;
      JsonPath *jpath = json_path_new ();

      json_path_compile (jpath,
                         path,
                         NULL);

      node = json_path_match (jpath,
                              json_parser_get_root (_parser));

      if (node)
      {
        JsonGenerator *generator = json_generator_new ();
        gchar         *quoted_str;

        json_generator_set_root (generator,
                                 node);

        quoted_str = json_generator_to_data (generator, NULL);

        if (quoted_str)
        {
          gchar **segments = g_strsplit_set (quoted_str, "\"[]", -1);

          if (segments)
          {
            for (guint i = 0; segments[i] != NULL; i++)
            {
              if (*segments[i] != '\0')
              {
                result = g_strdup (segments[i]);
                break;
              }
            }
          }

          g_strfreev (segments);
          g_free     (quoted_str);
        }

        g_object_unref (generator);
      }

      g_object_unref (jpath);
    }

    return result;
  }
}
