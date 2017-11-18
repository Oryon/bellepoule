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
  }

  // ----------------------------------------------------------------------------------------------
  Cryptor::~Cryptor ()
  {
    g_rand_free (_rand);
  }

  // ----------------------------------------------------------------------------------------------
  gchar *Cryptor::Encrypt (const gchar  *text,
                           const gchar  *key,
                           gchar       **iv_b64)
  {
    gchar          *base64_text;
    EVP_CIPHER_CTX *cipher      = EVP_CIPHER_CTX_new ();
    guint           text_len    = strlen (text) + 1;
    guchar         *cipher_txt  = g_new (guchar, text_len + AES_BLOCK_SIZE);
    gint            written_len;
    gint            cipher_len;

    {
      guchar *iv = GetIv ();

      EVP_CIPHER_CTX_init (cipher);

      EVP_EncryptInit_ex (cipher,
                          EVP_aes_256_cbc (),
                          NULL,
                          (guchar *) key,
                          iv);

      *iv_b64 = g_base64_encode (iv, 16);
      g_free (iv);
    }

    EVP_EncryptUpdate (cipher,
                       cipher_txt, &written_len,
                       (guchar *) text, text_len);
    cipher_len = written_len;

    EVP_EncryptFinal_ex (cipher,
                         cipher_txt + written_len,
                         &written_len);
    cipher_len += written_len;

    base64_text = g_base64_encode (cipher_txt,
                                   cipher_len);
    g_free (cipher_txt);

    EVP_CIPHER_CTX_cleanup (cipher);
    EVP_CIPHER_CTX_free (cipher);

    return base64_text;
  }

  // ----------------------------------------------------------------------------------------------
  gchar *Cryptor::Decrypt (gchar       *data,
                           const gchar *iv_b64,
                           const gchar *key)
  {
    if (iv_b64 && data)
    {
      gsize   cipher_len;
      guchar *cipher_bytes = g_base64_decode (data,
                                              &cipher_len);

      if (cipher_bytes)
      {
        EVP_CIPHER_CTX *cipher      = EVP_CIPHER_CTX_new ();
        gint            written_len;
        gsize           iv_len;
        guchar         *iv          = g_base64_decode (iv_b64, &iv_len);
        gint            plain_len;
        guchar         *plaintext   = g_new (guchar, cipher_len+1); // Including provision for
                                                                    // the missing NULL char

        EVP_CIPHER_CTX_init (cipher);

        EVP_DecryptInit_ex  (cipher,
                             EVP_aes_256_cbc (),
                             NULL,
                             (guchar *) key,
                             iv);

        EVP_DecryptUpdate (cipher,
                           plaintext, &written_len,
                           cipher_bytes, cipher_len);
        plain_len = written_len;

        EVP_DecryptFinal_ex (cipher,
                             plaintext + written_len,
                             &written_len);
        plain_len += written_len;
        plaintext[plain_len] = '\0';

        EVP_CIPHER_CTX_cleanup (cipher);
        EVP_CIPHER_CTX_free (cipher);

        g_free (cipher_bytes);
        g_free (iv);

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
}
