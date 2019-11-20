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

#include "server.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  Server::RequestBody::RequestBody (Server *server)
  {
    _server = server;
    _data   = nullptr;
    _length = 0;
  }

  // --------------------------------------------------------------------------------
  Server::RequestBody::~RequestBody ()
  {
    g_free (_data);
  }

  // --------------------------------------------------------------------------------
  void Server::RequestBody::ZeroTerminate ()
  {
    _data = (gchar *) g_realloc (_data,
                                 _length + 1);
    _data[_length] = '\0';
  }

  // --------------------------------------------------------------------------------
  void Server::RequestBody::Append (const char *buf,
                                    size_t      len)
  {
    _data = (gchar *) g_realloc (_data,
                                 _length + len);
    strncpy (&_data[_length],
             buf,
             len);
    _length += len;
  }

  // --------------------------------------------------------------------------------
  void Server::RequestBody::Replace (const char *buf)
  {
    g_free (_data);
    _data   = nullptr;
    _length = 0;

    if (buf)
    {
      Append (buf,
              strlen (buf) + 1);
    }
  }
}

namespace Net
{
  // --------------------------------------------------------------------------------
  Server::Server (Listener *listener,
                  guint     port)
    : Object ("Server")
  {
    _port     = port;
    _listener = listener;
  }

  // --------------------------------------------------------------------------------
  Server::~Server ()
  {
  }

  // --------------------------------------------------------------------------------
  guint Server::GetPort ()
  {
    return _port;
  }

  // --------------------------------------------------------------------------------
  void Server::Deliver (Message *message)
  {
    _listener->OnMessage (message);
  }
}
