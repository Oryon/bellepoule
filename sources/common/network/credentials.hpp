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

#include "util/flash_code.hpp"

namespace Net
{
  class Credentials : public FlashCode
  {
    public:
      Credentials (const gchar *name,
                   const gchar *ip_address,
                   guint        port,
                   const gchar *key);

      const gchar *GetKey ();

    private:
      guint  _port;
      gchar *_ip;
      gchar *_key;
      gchar *_name;

      ~Credentials () override;

      gchar *GetText () override;
  };
}
