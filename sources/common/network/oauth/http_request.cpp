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
#include "session.hpp"
#include "http_request.hpp"

namespace Oauth
{
  const gchar *HttpRequest::GET = "GET";

  const gchar HttpRequest::_nonce_range[] =
  {
    "0123456789"
    "abcdefghijklmnopqrstuvwyxz"
    "ABCDEFGHIJKLMNOPQRSTUVWYXZ"
  };

  // --------------------------------------------------------------------------------
  HttpRequest::HttpRequest (Session     *session,
                            const gchar *http_method,
                            const gchar *class_name)
    : Object (class_name)
  {
    _status = READY;

    _rand        = g_rand_new ();
    _header_list = NULL;

    _http_method = http_method;
    _session     = session;

    {
      gchar *nonce = GetNonce ();

      AddHeaderField ("oauth_consumer_key",     session->GetConsumerKey ());
      AddHeaderField ("oauth_nonce",            nonce);
      AddHeaderField ("oauth_signature_method", "HMAC-SHA1");
      AddHeaderField ("oauth_version",          "1.0");

      if (session->GetToken ())
      {
        AddHeaderField ("oauth_token", session->GetToken ());
      }

      g_free (nonce);
    }
  }

  // --------------------------------------------------------------------------------
  HttpRequest::~HttpRequest ()
  {
    g_list_free_full (_header_list,
                      (GDestroyNotify) Object::TryToRelease);
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
  void HttpRequest::ParseResponse (const gchar *response)
  {
    printf ("%s\n\n\n", response);

    if (response == NULL)
    {
      _status = NETWORK_ERROR;
    }
    else if (g_strstr_len (response,
                           -1,
                           "errors"))
    {
      _status = REJECTED;
    }
    else
    {
      gchar **fields = g_strsplit (response,
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
    gchar  *stamp     = g_strdup_printf ("%ld", real_time);

    AddHeaderField ("oauth_timestamp", stamp);
  }

  // --------------------------------------------------------------------------------
  gchar *HttpRequest::GetHeader ()
  {
    gchar *header;

    Stamp ();
    Sign  ();

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

    // Base string to sign
    {
      // Method
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
        GString *parameter_string = g_string_new (NULL);
        GList   *current          = _header_list;

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
}
