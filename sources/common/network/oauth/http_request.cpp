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

#include <stdio.h>
#include <string.h>

#include "field.hpp"
#include "v1_session.hpp"
#include "http_request.hpp"

namespace Oauth
{
  const gchar *HttpRequest::GET  = "GET";
  const gchar *HttpRequest::POST = "POST";

  const gchar HttpRequest::_nonce_range[] =
  {
    "0123456789"
    "abcdefghijklmnopqrstuvwyxz"
    "ABCDEFGHIJKLMNOPQRSTUVWYXZ"
  };

  // --------------------------------------------------------------------------------
  HttpRequest::HttpRequest (Session     *session,
                            const gchar *sub_url,
                            const gchar *http_method)
    : Object ("HttpRequest")
  {
    _status = READY;

    _rand           = g_rand_new ();
    _header_list    = NULL;
    _parameter_list = NULL;
    _parser         = NULL;
    _url            = g_strdup_printf ("%s/%s", session->GetApiUri (), sub_url);

    _http_method = http_method;
    _session     = (V1::Session *) session;

    {
      gchar *nonce = GetNonce ();

      AddHeaderField ("oauth_consumer_key",     _session->GetConsumerKey ());
      AddHeaderField ("oauth_nonce",            nonce);
      AddHeaderField ("oauth_signature_method", "HMAC-SHA1");
      AddHeaderField ("oauth_version",          "1.0");

      if (_session->GetToken ())
      {
        AddHeaderField ("oauth_token", _session->GetToken ());
      }

      g_free (nonce);
    }
  }

  // --------------------------------------------------------------------------------
  HttpRequest::~HttpRequest ()
  {
    g_free (_url);

    FreeFullGList (Field, _header_list);
    FreeFullGList (Field, _parameter_list);

    if (_parser)
    {
      g_object_unref (_parser);
    }
  }

  // --------------------------------------------------------------------------------
  const gchar *HttpRequest::GetURL ()
  {
    return _url;
  }

  // --------------------------------------------------------------------------------
  const gchar *HttpRequest::GetMethod ()
  {
    return _http_method;
  }

  // --------------------------------------------------------------------------------
  gboolean HttpRequest::LoadJson (const gchar *json)
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
  gchar *HttpRequest::GetJsonAtPath (const gchar *path)
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

  // --------------------------------------------------------------------------------
  gchar *HttpRequest::ExtractParsedField (const gchar *field_desc,
                                          const gchar *field_name)
  {
    if (g_strstr_len (field_desc,
                      -1,
                      field_name))
    {
      return g_strdup (&field_desc[strlen (field_name)]);
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  HttpRequest::Status HttpRequest::GetStatus ()
  {
    return _status;
  }

  // --------------------------------------------------------------------------------
  void HttpRequest::ParseResponse (GHashTable  *header,
                                   const gchar *body)
  {
    // printf ("%s\n\n\n", body);

    DumpRateLimits (header);

    if (body == NULL)
    {
      _status = NETWORK_ERROR;
    }
    else if (g_strstr_len (body,
                           -1,
                           "errors"))
    {
      _status = REJECTED;
    }
    else
    {
      gchar **fields = g_strsplit (body,
                                   "&",
                                   -1);

      if (fields)
      {
        gchar **current = fields;

        while (current[0])
        {
          gchar *field;

          field = ExtractParsedField (current[0],
                                      "oauth_token=");
          if (field)
          {
            _session->SetToken (field);
          }

          field = ExtractParsedField (current[0],
                                      "oauth_token_secret=");
          if (field)
          {
            _session->SetTokenSecret (field);
          }

          current++;
        }

        g_strfreev (fields);
      }
      _status = ACCEPTED;
    }
  }

  // --------------------------------------------------------------------------------
  void HttpRequest::DumpRateLimits (GHashTable  *header)
  {
    guint        rate_limit_limit;
    guint        rate_limit_remaining;
    GDateTime   *rate_limit_reset = NULL;
    const gchar *value;

    value = (const gchar *) g_hash_table_lookup (header,
                                                 "x-rate-limit-limit");
    if (value)
    {
      rate_limit_limit = g_ascii_strtoull (value,
                                           NULL,
                                           10);
    }

    value = (const gchar *) g_hash_table_lookup (header,
                                                 "x-rate-limit-remaining");
    if (value)
    {
      rate_limit_remaining = g_ascii_strtoull (value,
                                               NULL,
                                               10);
    }

    value = (const gchar *) g_hash_table_lookup (header,
                                                 "x-rate-limit-reset");
    if (value)
    {
      {
        GTimeVal time_val;

        time_val.tv_sec = g_ascii_strtoull (value,
                                            NULL,
                                            10);
        time_val.tv_usec = 0;
        rate_limit_reset = g_date_time_new_from_timeval_utc (&time_val);
      }
    }

    if (rate_limit_reset)
    {
      GDateTime *now   = g_date_time_new_now_utc ();
      GTimeSpan  delay = g_date_time_difference (rate_limit_reset, now);
      GDateTime *reset_time;
      gchar     *reset_str;

      {
        GTimeVal tv;

        tv.tv_sec  = delay / G_TIME_SPAN_SECOND;
        tv.tv_usec = 0;

        reset_time = g_date_time_new_from_timeval_utc (&tv);
      }

      reset_str = g_date_time_format (reset_time, "%H:%M:%S");
      printf ("%s ==> " BLUE "%d/%d (%s)\n" ESC, GetURL (), rate_limit_remaining, rate_limit_limit, reset_str);

      g_date_time_unref (reset_time);
      g_date_time_unref (now);
      g_free (reset_str);
      g_date_time_unref (rate_limit_reset);
    }
  }

  // --------------------------------------------------------------------------------
  void HttpRequest::ForgiveError ()
  {
    _status = ACCEPTED;
  }

  // --------------------------------------------------------------------------------
  void HttpRequest::AddHeaderField (const gchar *key,
                                    const gchar *value)
  {
    Field *field = new Field (key,
                              value);

    _header_list = g_list_insert_sorted (_header_list,
                                         field,
                                         (GCompareFunc) Field::Compare);
  }

  // --------------------------------------------------------------------------------
  void HttpRequest::AddParameterField (const gchar *key,
                                       const gchar *value)
  {
    Field *field = new Field (key,
                              value);

    _parameter_list = g_list_insert_sorted (_parameter_list,
                                            field,
                                            (GCompareFunc) Field::Compare);
  }

  // --------------------------------------------------------------------------------
  char *HttpRequest::GetNonce ()
  {
    const guint  nonce_length = 32;
    gchar       *nonce        = g_new0 (gchar, nonce_length+1);

    for (guint i = 0; i < nonce_length; i++)
    {
      guint random_index = g_rand_int_range (_rand,
                                             0,
                                             sizeof (_nonce_range));

      nonce[i] = _nonce_range[random_index];
    }

    return nonce;
  }

  // --------------------------------------------------------------------------------
  void HttpRequest::Stamp ()
  {
    gint64  real_time = g_get_real_time () / G_USEC_PER_SEC;
    gchar  *stamp     = g_strdup_printf ("%" G_GINT64_FORMAT, real_time);

    AddHeaderField ("oauth_timestamp", stamp);
  }

  // --------------------------------------------------------------------------------
  gchar *HttpRequest::GetHeader ()
  {
    gchar *header;

    Stamp ();
    Sign ();

    {
      GString *header_string = g_string_new ("Authorization: OAuth ");

      // Fields
      {
        GList *current = _header_list;

        for (guint i = 0; current; i++)
        {
          Field *field = (Field *) current->data;

          if (i > 0)
          {
            header_string = g_string_append (header_string,
                                             ", ");
          }
          header_string = g_string_append (header_string,
                                           field->GetQuotedParcel ());

          current = g_list_next (current);
        }

        header = header_string->str;
        g_string_free (header_string,
                       FALSE);
      }
    }

    return header;
  }

  // --------------------------------------------------------------------------------
  void HttpRequest::Sign ()
  {
    GString *base_string = g_string_new (NULL);
    gchar   *signature;

    // String to sign
    {
      base_string = g_string_append (base_string,
                                     _http_method);
      base_string = g_string_append_c (base_string,
                                       '&');

      // URL
      {
        gchar *encoded_url = g_uri_escape_string (GetURL (),
                                                  NULL,
                                                  FALSE);

        base_string = g_string_append (base_string,
                                       encoded_url);
        base_string = g_string_append_c (base_string,
                                         '&');

        g_free (encoded_url);
      }

      // Fields
      {
        GList   *current;
        GString *parameter_string = g_string_new (NULL);
        GList   *signing_list     = g_list_concat (g_list_copy (_header_list),
                                                   g_list_copy (_parameter_list));
        signing_list = g_list_sort (signing_list,
                                    (GCompareFunc) Field::Compare);
        current = signing_list;
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

        {
          gchar *encoded = g_uri_escape_string (parameter_string->str,
                                                NULL,
                                                FALSE);

          base_string = g_string_append (base_string,
                                         encoded);

          g_free (encoded);
        }

        g_string_free (parameter_string,
                       TRUE);
        g_list_free (signing_list);
      }
    }

    // Signature
    {
      gchar        *hmac_string;
      guint         hmac_len;
      guchar       *hmac_hexa;
      const guchar *signing_key = _session->GetSigningKey ();

      hmac_string = g_compute_hmac_for_string (G_CHECKSUM_SHA1,
                                               signing_key,
                                               strlen ((const gchar *) signing_key),
                                               base_string->str,
                                               -1);
      hmac_len  = strlen (hmac_string);
      hmac_hexa = g_new (guchar, hmac_len/2);

      for (guint i = 0; i < hmac_len / 2; i++)
      {
        gchar ascii[3];

        g_strlcpy (ascii,
                   &hmac_string[i*2],
                   sizeof (ascii));
        hmac_hexa[i] = g_ascii_strtoull (ascii,
                                         NULL,
                                         16);
      }

      signature = g_base64_encode (hmac_hexa,
                                   hmac_len/2);

      g_free (hmac_hexa);
      g_free (hmac_string);

      g_string_free (base_string,
                     TRUE);
    }

    AddHeaderField ("oauth_signature", signature);
    g_free (signature);
  }

  // --------------------------------------------------------------------------------
  gchar *HttpRequest::GetParameters ()
  {
    gchar *parameters = NULL;

    if (_parameter_list)
    {
      GList   *current          = _parameter_list;
      GString *parameter_string = g_string_new (NULL);

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
}
