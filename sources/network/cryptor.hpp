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

#ifndef cryptor_hpp
#define cryptor_hpp

#include <openssl/aes.h>
#include <openssl/evp.h>

#include "util/object.hpp"

namespace Net
{
  class Cryptor : public Object
  {
    public:
      Cryptor ();

      guchar *Encrypt (const gchar *text,
                       const gchar *key,
                       guint       *length);

      gchar *Decrypt (gchar        *data,
                      const guchar *_iv,
                      const gchar  *key);

    private:
      GRand          *_rand;
      EVP_CIPHER_CTX  _en_cipher;
      EVP_CIPHER_CTX  _de_cipher;

      virtual ~Cryptor ();

      guchar *GetIv ();

      guchar *GetBytes (gchar *data,
                        gint   bytes_count);
  };
}

#endif
