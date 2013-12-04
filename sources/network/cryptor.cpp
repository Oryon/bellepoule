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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/aes.h>
#include <openssl/evp.h>

#include "cryptor.hpp"

namespace Net
{
  // ----------------------------------------------------------------------------------------------
  Cryptor::Cryptor ()
    : Object ("Cryptor")
  {
    _rand = g_rand_new ();

    EVP_CIPHER_CTX_init (&_en_cipher);
    EVP_CIPHER_CTX_init (&_de_cipher);
  }

  // ----------------------------------------------------------------------------------------------
  Cryptor::~Cryptor ()
  {
    EVP_CIPHER_CTX_cleanup (&_en_cipher);
    EVP_CIPHER_CTX_cleanup (&_de_cipher);

    g_rand_free (_rand);
  }

  // ----------------------------------------------------------------------------------------------
  guchar *Cryptor::Encrypt (const gchar *text,
                            const gchar *key,
                            guint       *length)
  {
    // max cipher_txt len for a n bytes of text is n + AES_BLOCK_SIZE -1 bytes
    guint   text_len   = strlen ((gchar *) text);
    gint    cipher_len = text_len + AES_BLOCK_SIZE;
    guchar *cipher_txt = (guchar *) malloc (cipher_len);
    guchar *iv         = GetIv ();

    EVP_EncryptInit_ex  (&_en_cipher,
                         EVP_aes_256_cbc (),
                         NULL,
                         (guchar *) key,
                         iv);

    EVP_EncryptUpdate (&_en_cipher,
                       cipher_txt, &cipher_len,
                       (guchar *) text, text_len);

    {
      gint remaining_len;

      EVP_EncryptFinal_ex (&_en_cipher,
                           cipher_txt + cipher_len,
                           &remaining_len);

      *length = cipher_len + remaining_len;
    }

    return cipher_txt;
  }

  // ----------------------------------------------------------------------------------------------
  gchar *Cryptor::Decrypt (gchar        *data,
                           const guchar *iv,
                           const gchar  *key)
  {
    if (iv && data)
    {
      gsize   bytes_len;
      guchar *data_bytes = g_base64_decode (data,
                                            &bytes_len);

      if (iv && data_bytes)
      {
        gint    plain_len = bytes_len;
        guchar *plaintext;

        // because we have padding ON, we must allocate an extra cipher block size of memory
        plaintext = g_new (guchar, plain_len + AES_BLOCK_SIZE);

        EVP_DecryptInit_ex  (&_de_cipher,
                             EVP_aes_256_cbc (),
                             NULL,
                             (guchar *) key,
                             iv);

        EVP_DecryptUpdate (&_de_cipher,
                           plaintext, &plain_len,
                           data_bytes, bytes_len);

        {
          gint remaining_len;

          EVP_DecryptFinal_ex (&_de_cipher,
                               plaintext + plain_len,
                               &remaining_len);
        }

        g_free (data_bytes);

        return (gchar *) plaintext;
      }
    }

    return NULL;
  }

  // ----------------------------------------------------------------------------------------------
  guchar *Cryptor::GetIv ()
  {
    guchar *iv = g_new (guchar, 16);

    for (guint i = 0; i < 16; i++)
    {
      iv[i] = (guchar) g_rand_int (_rand);
    }

    return iv;
  }

  // ----------------------------------------------------------------------------------------------
  guchar *Cryptor::GetBytes (gchar *data,
                             gint   bytes_count)
  {
    guchar *bytes = NULL;

    if ((bytes_count % 2) == 0)
    {
      gchar *current_symbol = data;

      bytes = g_new (guchar, bytes_count/2);

      for (gint i = 0; i < bytes_count; i++)
      {
        if (current_symbol[0] && current_symbol[1])
        {
          gchar symbol[3];

          symbol[0] = current_symbol[0];
          symbol[1] = current_symbol[1];
          symbol[2] = '\0';

          bytes[i] = (guchar) strtol (symbol,
                                      NULL,
                                      16);
          current_symbol += 2;
        }
      }
    }

    return bytes;
  }
}
