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

#include <glib.h>
#include <microhttpd.h>

#include "util/object.hpp"

namespace Net
{
  class Server : public Object
  {
    public:
      struct Listener
      {
          virtual const gchar *GetSecretKey (const gchar *authentication_scheme) = 0;
          virtual gboolean     OnMessage    (Message     *message)               = 0;
      };

      guint GetPort ();

    protected:
      Server (Listener *listener,
              guint     port);

      void Deliver (Message *message);

    protected:
      struct RequestBody
      {
        RequestBody (Server *server);
        virtual ~RequestBody ();

        void Append (const char *buf,
                     size_t      len);
        void Replace (const char *buf);
        void ZeroTerminate ();

        gchar  *_data;
        guint   _length;
        Server *_server;
      };

      Listener *_listener;

      ~Server () override;

    private:
      guint _port;
  };
}
