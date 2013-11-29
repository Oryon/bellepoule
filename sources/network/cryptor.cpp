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
                            const gchar *key)
  {
    // max cipher_txt len for a n bytes of text is n + AES_BLOCK_SIZE -1 bytes
    guint   text_len   = strlen ((gchar *) text);
    gint    cipher_len = text_len + AES_BLOCK_SIZE;
    guchar *cipher_txt = (guchar *) malloc (cipher_len);
    guchar *iv         = GetIv ();

    EVP_EncryptInit_ex  (&_en_cipher, EVP_aes_256_cbc (), NULL, (guchar *) key, iv);

    EVP_EncryptUpdate (&_en_cipher,
                       cipher_txt, &cipher_len,
                       (guchar *) text, text_len);

    {
      gint remaining_len;

      EVP_EncryptFinal_ex (&_en_cipher,
                           cipher_txt + cipher_len,
                           &remaining_len);

      //*text_len = cipher_len + remaining_len;
    }

    return cipher_txt;
  }

  // ----------------------------------------------------------------------------------------------
  guchar *Cryptor::Decrypt (gchar       *text,
                            const gchar *key)
  {
    gchar *body = strchr (text, '/');

    if (body)
    {
      gint    plain_len;
      gint    iv_len;
      gint    body_len;
      guchar *plaintext;
      guchar *iv_bytes;
      guchar *body_bytes;

      *body = '\0';
      body++;

      iv_bytes   = GetBytes (text, &iv_len);
      body_bytes = GetBytes (body, &body_len);

      // because we have padding ON, we must allocate an extra cipher block size of memory
      plain_len = body_len;
      plaintext = (guchar *) malloc (plain_len + AES_BLOCK_SIZE);

      EVP_DecryptInit_ex  (&_de_cipher, EVP_aes_256_cbc (), NULL, (guchar *) key, iv_bytes);

      EVP_DecryptUpdate (&_de_cipher,
                         (guchar *) plaintext, &plain_len,
                         body_bytes, body_len);

      {
        gint remaining_len;

        EVP_DecryptFinal_ex (&_de_cipher,
                             plaintext + plain_len,
                             &remaining_len);

        //*len = plain_len + remaining_len;
      }

      return plaintext;
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
  guchar *Cryptor::GetBytes (gchar *text,
                             gint  *bytes_count)
  {
    gchar  *current_symbol = text;
    guchar *bytes;
    guchar *current_byte;

    *bytes_count = strlen (text)/2;
    bytes        = g_new (guchar, *bytes_count);
    current_byte = bytes;

    while (current_symbol)
    {
      if (current_symbol[0] && current_symbol[1])
      {
        gchar symbol[3];

        symbol[0] = current_symbol[0];
        symbol[1] = current_symbol[1];
        symbol[2] = '\0';

        *current_byte = (guchar) strtol (symbol,
                                         NULL,
                                         16);
        current_symbol += 2;
        current_byte   += 1;
      }
      else
      {
        break;
      }
    }

    return bytes;
  }
}
