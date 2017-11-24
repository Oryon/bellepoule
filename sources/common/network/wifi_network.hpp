// Copyright (C) 2011 Yannick Le Roux.
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
  class WifiNetwork : public Object
  {
    public:
      WifiNetwork ();

      gboolean IsValid ();

      const gchar *GetSSID ();

      const gchar *GetPassphrase ();

      const gchar *GetEncryption ();

      void SetSSID (const gchar *SSID);

      void SetPassphrase (const gchar *passphrase);

      void SetEncryption (const gchar *encryption);

    private:
      gchar *_SSID;
      gchar *_passphrase;
      gchar *_encryption;

      virtual ~WifiNetwork ();
  };
}
