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

#ifndef partner_hpp
#define partner_hpp

#include "util/object.hpp"

namespace Net
{
  class Message;

  class Partner : public Object
  {
    public:
      Partner (Message *message);

      gboolean SendMessage (Message *message);

      gboolean HasSameRole (Partner *than);

      static gint CompareRole (Partner     *partner,
                               const gchar *with);

    private:
      gchar *_ip;
      gchar *_role;
      guint  _port;

      ~Partner ();
  };
}

#endif
