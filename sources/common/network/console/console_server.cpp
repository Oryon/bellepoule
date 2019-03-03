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

#include "console_server.hpp"

namespace Net
{
  GHashTable *ConsoleServer::_variables     = nullptr;
  guint       ConsoleServer::_longuest_name = 0;
  gchar      *ConsoleServer::_padder        = nullptr;

  // --------------------------------------------------------------------------------
  ConsoleServer::ConsoleServer (guint port)
    : Object ("ConsoleServer")
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
  ConsoleServer::~ConsoleServer ()
  {
    g_source_destroy (_connection_source);

    for (GList *current = _clients; current; current = g_list_next (current))
    {
      Console *console = (Console *) current->data;

      console->Release ();
    }
    g_list_free (_clients);
  }

  // --------------------------------------------------------------------------------
  gboolean ConsoleServer::OnNewClient (GSocket       *socket,
                                       GIOCondition   condition,
                                       ConsoleServer *server)
  {
    if (condition == G_IO_IN)
    {
      Console *console;
      GSocket *client_socket = g_socket_accept (socket,
                                                nullptr,
                                                nullptr);

      console = new Console (client_socket,
                             server);

      server->_clients = g_list_prepend (server->_clients,
                                         console);
    }

    return G_SOURCE_CONTINUE;
  }

  // --------------------------------------------------------------------------------
  void ConsoleServer::ExposeVariable (const gchar *variable,
                                      guint        initial_value)
  {
    if (_variables == nullptr)
    {
      _variables = g_hash_table_new (g_str_hash,
                                     g_str_equal);
    }

    g_hash_table_insert (_variables,
                         (gpointer) variable,
                         GUINT_TO_POINTER (initial_value));

    {
      guint len = strlen (variable);

      if (len > _longuest_name)
      {
        _longuest_name = len;
        _padder = g_strdup_printf ("%%-%ds", _longuest_name);
      }
    }
  }

  // --------------------------------------------------------------------------------
  gboolean ConsoleServer::VariableIsSet (const gchar *variable)
  {
    if (_variables)
    {
      return GPOINTER_TO_UINT (g_hash_table_lookup (_variables,
                                                    variable)) != 0;
    }

    return TRUE;
  }

  // --------------------------------------------------------------------------------
  void ConsoleServer::OnCommand (Console     *console,
                                 const gchar *command)
  {
    gchar **args = g_strsplit (g_strstrip ((gchar *) command),
                               " ",
                               -1);

    if (args)
    {
      if (CommandIs (args[0],
                     "help"))
      {
        console->Echo (BLUE " r" ESC "etained       dump a summary of the retained objects.\n"
                       BLUE " v" ESC "ars           list all the variables.\n"
                       BLUE " t" ESC "oggle var     toggle the given variable.\n");
      }
      else if (CommandIs (args[0],
                          "retained"))
      {
        Object::DumpList ();
      }
      else if (CommandIs (args[0],
                          "vars"))
      {
        if (_variables)
        {
          GHashTableIter iter;
          gpointer       key;
          gpointer       value;

          g_hash_table_iter_init (&iter,
                                  _variables);

          while (g_hash_table_iter_next (&iter,
                                         &key,
                                         &value))
          {
            EchoVariable (console,
                          (gchar *) key,
                          GPOINTER_TO_UINT (value));
          }
        }
      }
      else if (CommandIs (args[0],
                          "toggle"))
      {
        if (args[1])
        {
          if (_variables)
          {
            const gchar    *candidate = nullptr;
            guint           new_value;
            GHashTableIter  iter;
            gpointer        key;
            gpointer        value;

            g_hash_table_iter_init (&iter,
                                    _variables);

            while (g_hash_table_iter_next (&iter,
                                           &key,
                                           &value))
            {
              if (g_strrstr ((gchar *) key,
                             args[1]) == key)
              {
                if (candidate)
                {
                  candidate = nullptr;
                  break;
                }
                candidate = (gchar *) key;
                if (value)
                {
                  new_value = 0;
                }
                else
                {
                  new_value = 1;
                }
              }
            }

            if (candidate)
            {
              g_hash_table_insert (_variables,
                                   (gpointer) candidate,
                                   GUINT_TO_POINTER (new_value));
              EchoVariable (console,
                            (gchar *) key,
                            new_value);
            }
            else
            {
              console->Echo ("Unknown variable\n");
            }
          }
        }
      }

      g_strfreev (args);
    }
  }

  // --------------------------------------------------------------------------------
  void ConsoleServer::OnShellClosed (Console *console)
  {
    _clients = g_list_remove (_clients,
                              console);
    console->Release ();
  }

  // --------------------------------------------------------------------------------
  void ConsoleServer::EchoVariable (Console     *console,
                                    const gchar *name,
                                    guint        value)
  {
    {
      gchar *padded = g_strdup_printf (_padder, name);

      console->Echo (padded);
      g_free (padded);
    }

    if (value)
    {
      console->Echo (" [" GREEN "*" ESC "]");
    }
    else
    {
      console->Echo (" [" RED "-" ESC "]");
    }
    console->Echo ("\n");
  }

  // --------------------------------------------------------------------------------
  gboolean ConsoleServer::CommandIs (const gchar *command,
                                     const gchar *name)
  {
    if ((command == nullptr) || (command[0] == '\0'))
    {
      return FALSE;
    }

    if (command[0] != name[0])
    {
      return FALSE;
    }

    if (command[1] == '\0')
    {
      return TRUE;
    }

    return (g_strcmp0 (command,
                       name) == 0);
  }
}
