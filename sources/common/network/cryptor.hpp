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

#include "util/object.hpp"

namespace Net
{
  class Cryptor : public Object
  {
    public:
      Cryptor (const gchar *iv_b64 = nullptr);

      gchar *Encrypt (const gchar  *text,
                      const gchar  *key,
                      gchar       **iv_b64);

      gchar *Decrypt (gchar       *data_b64,
                      const gchar *iv_b64,
                      const gchar *key);

    private:
      gchar *_iv;

      ~Cryptor () override;

      guchar *GenerateIv ();
  };
}
