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

#include <sys/types.h>
#include <qrencode.h>

#include "util/flash_code.hpp"

#include "ring.hpp"
#include "credentials.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  Credentials::Credentials (const gchar *name,
                            const gchar *ip_address,
                            guint        port,
                            const gchar *key)
    : FlashCode (NULL)
  {
    _name = g_strdup (name);
    _ip   = g_strdup (ip_address);
    _key  = g_strdup (key);
    _port = port;
  }

  // --------------------------------------------------------------------------------
  Credentials::~Credentials ()
  {
    g_free (_name);
    g_free (_ip);
    g_free (_key);
  }

  // --------------------------------------------------------------------------------
  gchar *Credentials::GetKey ()
  {
    return g_strdup ((gchar *) _key);
  }

  // --------------------------------------------------------------------------------
  gchar *Credentials::GetText ()
  {
    return g_strdup_printf ("%s:%s:%d:%s",
                            _name,
                            _ip,
                            _port,
                            _key);
  }
}
