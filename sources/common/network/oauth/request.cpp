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

#include <curl/curl.h>

#include "session.hpp"
#include "field.hpp"
#include "request.hpp"

namespace Oauth
{
  const gchar *Request::HTTP_GET    = "GET";
  const gchar *Request::HTTP_POST   = "POST";
  const gchar *Request::HTTP_DELETE = "DELETE";

  // --------------------------------------------------------------------------------
  Request::Request (Session     *session,
                    const gchar *signature,
                    const gchar *http_method)
    : Object ("Oauth::Request")
  {
    _session        = session;
    _status         = READY;
    _http_method    = http_method;
    _url            = g_strdup_printf ("%s/%s", session->GetApiUri (), signature);
    _header_list    = nullptr;
    _parameter_list = nullptr;
    _parser         = nullptr;
  }

  // --------------------------------------------------------------------------------
  Request::~Request ()
  {
    FreeFullGList (Field, _header_list);
    FreeFullGList (Field, _parameter_list);

    g_free (_url);

    if (_parser)
    {
      g_object_unref (_parser);
    }
  }

  // --------------------------------------------------------------------------------
  void Request::SetSignature (const gchar *signature)
  {
    g_free (_url);
    _url = g_strdup_printf ("%s/%s", _session->GetApiUri (), signature);
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
  void Request::AddHeaderField (const gchar *key,
                                const gchar *value)
  {
    if (value)
    {
      Field *field = new Field (key,
                                value);

      {
        GList *current = _header_list;

        while (current)
        {
          Field *current_field = (Field *) current->data;

          if (Field::Compare (current_field, field) == 0)
          {
            _header_list = g_list_delete_link (_header_list,
                                               current);
            break;
          }
          current = g_list_next (current);
        }
      }

        _header_list = g_list_insert_sorted (_header_list,
                                             field,
                                             (GCompareFunc) Field::Compare);
    }
  }

  // --------------------------------------------------------------------------------
  void Request::AddParameterField (const gchar *key,
                                   const gchar *value)
  {
    if (value)
    {
      Field *field = new Field (key,
                                value);

      _parameter_list = g_list_insert_sorted (_parameter_list,
                                              field,
                                              (GCompareFunc) Field::Compare);
    }
  }

  // --------------------------------------------------------------------------------
  gchar *Request::GetParameters ()
  {
    gchar *parameters = nullptr;

    if (_parameter_list)
    {
      GList   *current          = _parameter_list;
      GString *parameter_string = g_string_new (nullptr);

      for (guint i = 0; current; i++)
      {
        Field *field = (Field *) current->data;

        if (i > 0)
        {
          parameter_string = g_string_append_c (parameter_string,
                                                '&');
        }

        parameter_string = g_string_append (parameter_string,
                                            field->GetParcel ());

        current = g_list_next (current);
      }

      parameters = parameter_string->str;
      g_string_free (parameter_string,
                     FALSE);
    }

    return parameters;
  }

  // --------------------------------------------------------------------------------
  gboolean Request::LoadJson (const gchar *json)
  {
    if (_parser == nullptr)
    {
      _parser = json_parser_new ();
    }

    return json_parser_load_from_data (_parser,
                                       json,
                                       -1,
                                       nullptr);
  }

  // --------------------------------------------------------------------------------
  gchar *Request::GetJsonAtPath (const gchar *path)
  {
    gchar *result = nullptr;

    if (_parser)
    {
      JsonNode *node;
      JsonPath *jpath = json_path_new ();

      json_path_compile (jpath,
                         path,
                         nullptr);

      node = json_path_match (jpath,
                              json_parser_get_root (_parser));

      if (node)
      {
        JsonGenerator *generator = json_generator_new ();
        gchar         *quoted_str;

        json_generator_set_root (generator,
                                 node);

        quoted_str = json_generator_to_data (generator, nullptr);

        if (quoted_str)
        {
          gchar **segments = g_strsplit_set (quoted_str, "\"[]", -1);

          if (segments)
          {
            for (guint i = 0; segments[i] != nullptr; i++)
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

  // --------------------------------------------------------------------------------
  struct curl_slist *Request::GetHeader ()
  {
    if (_header_list)
    {
      struct curl_slist *curl_header = nullptr;
      GList             *current     = _header_list;

      while (current)
      {
        Field *field = (Field *) current->data;

        curl_header = curl_slist_append (curl_header,
                                         field->GetHeaderForm ());


        current = g_list_next (current);
      }

      return curl_header;
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  void Request::ParseResponse (GHashTable  *header,
                               const gchar *body)
  {
    printf ("%s\n\n\n", body);

    if (body == nullptr)
    {
      _status = NETWORK_ERROR;
    }
    else if (g_strstr_len (body,
                           -1,
                           "error"))
    {
      _status = REJECTED;
    }
    else
    {
      _status = ACCEPTED;
    }
  }
}
