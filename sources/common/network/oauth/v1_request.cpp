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
#include <curl/curl.h>

#include "field.hpp"
#include "v1_session.hpp"
#include "v1_request.hpp"

namespace Oauth
{
  namespace V1
  {
    const gchar Request::_nonce_range[] =
    {
      "0123456789"
      "abcdefghijklmnopqrstuvwyxz"
      "ABCDEFGHIJKLMNOPQRSTUVWYXZ"
    };

    // --------------------------------------------------------------------------------
    Request::Request (Oauth::Session *session,
                      const gchar    *sub_url,
                      const gchar    *http_method)
      : Object ("Oauth::V1::Request"),
        Oauth::Request (session, sub_url, http_method)
    {
      _rand = g_rand_new ();
    }

    // --------------------------------------------------------------------------------
    Request::~Request ()
    {
    }

    // --------------------------------------------------------------------------------
    Session *Request::GetSession ()
    {
      return (Session *) _session;
    }

    // --------------------------------------------------------------------------------
    gchar *Request::ExtractParsedField (const gchar *field_desc,
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
    void Request::ParseResponse (GHashTable  *header,
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
            Session *session = GetSession ();
            gchar   *field;

            field = ExtractParsedField (current[0],
                                        "oauth_token=");
            if (field)
            {
              session->SetToken (field);
            }

            field = ExtractParsedField (current[0],
                                        "oauth_token_secret=");
            if (field)
            {
              session->SetTokenSecret (field);
            }

            current++;
          }

          g_strfreev (fields);
        }
        _status = ACCEPTED;
      }
    }

    // --------------------------------------------------------------------------------
    void Request::DumpRateLimits (GHashTable *header)
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
      else
      {
        return;
      }

      value = (const gchar *) g_hash_table_lookup (header,
                                                   "x-rate-limit-remaining");
      if (value)
      {
        rate_limit_remaining = g_ascii_strtoull (value,
                                                 NULL,
                                                 10);
      }
      else
      {
        return;
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
        g_print (YELLOW "%s ==> " ESC "%d/%d (%s)\n",
                 _session->GetService (),
                 rate_limit_remaining, rate_limit_limit, reset_str);

        g_date_time_unref (reset_time);
        g_date_time_unref (now);
        g_free (reset_str);
        g_date_time_unref (rate_limit_reset);
      }
    }

    // --------------------------------------------------------------------------------
    char *Request::GetNonce ()
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
    void Request::Stamp ()
    {
      gint64  real_time = g_get_real_time () / G_USEC_PER_SEC;
      gchar  *stamp     = g_strdup_printf ("%" G_GINT64_FORMAT, real_time);

      AddHeaderField ("oauth_timestamp", stamp);
    }

    // --------------------------------------------------------------------------------
    void Request::Sign ()
    {
      GString *base_string = g_string_new (NULL);
      gchar   *signature;

      // String to sign
      {
        base_string = g_string_append (base_string,
                                       GetMethod ());
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
        Session      *session     = GetSession ();
        const guchar *signing_key = session->GetSigningKey ();

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
    struct curl_slist *Request::GetHeader ()
    {
      gchar *bundle = NULL;

      {
        Session *v1_session = GetSession ();
        gchar   *nonce      = GetNonce ();

        AddHeaderField ("oauth_consumer_key",     v1_session->GetConsumerKey ());
        AddHeaderField ("oauth_nonce",            nonce);
        AddHeaderField ("oauth_signature_method", "HMAC-SHA1");
        AddHeaderField ("oauth_version",          "1.0");

        if (v1_session->GetToken ())
        {
          AddHeaderField ("oauth_token", v1_session->GetToken ());
        }

        g_free (nonce);
      }

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

          bundle = header_string->str;
          g_string_free (header_string,
                         FALSE);
        }
      }

      if (bundle)
      {
        struct curl_slist *curl_header = NULL;

        curl_header = curl_slist_append (curl_header, bundle);
        g_free (bundle);
        return curl_header;
      }

      return NULL;
    }
  }
}
