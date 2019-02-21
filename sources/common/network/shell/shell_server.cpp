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

#include "shell_server.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  ShellServer::ShellServer (guint port)
    : Object ("ShellServer")
  {
    GError  *error             = nullptr;
    GSocket *connection_socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
                                               G_SOCKET_TYPE_STREAM,
                                               G_SOCKET_PROTOCOL_DEFAULT,
                                               nullptr);

    {
      GInetAddress   *any_ip        = g_inet_address_new_any    (G_SOCKET_FAMILY_IPV4);
      GSocketAddress *bound_address = g_inet_socket_address_new (any_ip, port);

      g_socket_bind (connection_socket,
                     bound_address,
                     TRUE,
                     &error);

      g_object_unref (any_ip);
      g_object_unref (bound_address);
    }

    if (error)
    {
      g_warning ("g_socket_bind: %s\n", error->message);
      g_clear_error (&error);
    }
    else if (g_socket_listen (connection_socket,
                              nullptr))
    {
      _connection_source = g_socket_create_source (connection_socket,
                                                   (GIOCondition) (G_IO_IN|G_IO_ERR|G_IO_HUP),
                                                   nullptr);

      g_source_set_callback (_connection_source,
                             (GSourceFunc) OnNewClient,
                             this,
                             nullptr);
      g_source_attach (_connection_source,
                       nullptr);
    }

    g_object_unref (connection_socket);
  }

  // --------------------------------------------------------------------------------
  ShellServer::~ShellServer ()
  {
    g_source_destroy (_connection_source);

    for (GList *current = _clients; current; current = g_list_next (current))
    {
      Shell *shell = (Shell *) current->data;

      shell->Release ();
    }
    g_list_free (_clients);
  }

  // --------------------------------------------------------------------------------
  gboolean ShellServer::OnNewClient (GSocket      *socket,
                                     GIOCondition  condition,
                                     ShellServer  *server)
  {
    if (condition == G_IO_IN)
    {
      Shell   *shell;
      GSocket *client_socket = g_socket_accept (socket,
                                                nullptr,
                                                nullptr);

      shell = new Shell (client_socket,
                         server);

      server->_clients = g_list_prepend (server->_clients,
                                         shell);
    }

    return G_SOURCE_CONTINUE;
  }

  // --------------------------------------------------------------------------------
  void ShellServer::OnShellClosed (Shell *shell)
  {
    _clients = g_list_remove (_clients,
                              shell);
    shell->Release ();
  }
}
