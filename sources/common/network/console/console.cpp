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

#include "console.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  Console::Console (GSocket  *socket,
                    Listener *listener)
    : Object ("Console")
  {
    _command  = g_string_new ("");
    _listener = listener;
    _socket   = socket;

    {
      _source = g_socket_create_source (_socket,
                                        (GIOCondition) (G_IO_IN|G_IO_ERR|G_IO_HUP),
                                        nullptr);

      g_source_set_callback (_source,
                             (GSourceFunc) OnSocketEvent,
                             this,
                             nullptr);
      g_source_attach (_source,
                       nullptr);
    }

    {
      const gchar *logo =
        "                              \n"
        "             .dkkOOOOkk:.     \n"
        "             .O00KXXKx,       \n"
        "             .0KK0Kx,         \n"
        "             .0K0xol:.        \n"
        "             .00kkOOOOo.      \n"
        "             .O00K0K000Ol.    \n"
        "           .lxddkKK000000'    \n"
        "         'd0KOdollx00OO00'    \n"
        "       'oO000Oxddd:,cxOOO'    \n"
        "     'lkOO000Okxc.    ,dO'    \n"
        "   ;d000OOOOOOc.        ,.    \n"
        "   ...........                \n"
        "                              \n";

      Echo       (logo);
      EchoPrompt ();
    }
  }

  // --------------------------------------------------------------------------------
  Console::~Console ()
  {
    g_string_free (_command,
                   TRUE);

    g_source_destroy (_source);
    g_object_unref   (_socket);
  }

  // --------------------------------------------------------------------------------
  void Console::Echo (const gchar *message)
  {
    for (size_t size = strlen (message) + 1; size > 0;)
    {
      int     sent;
      GError *error = nullptr;

      sent = g_socket_send (_socket,
                            message,
                            size,
                            nullptr,
                            &error);

      if (error)
      {
        g_warning ("g_socket_send: %s\n", error->message);
        g_clear_error (&error);
        return;
      }
      else
      {
        message += sent;
        size    -= sent;
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Console::EchoPrompt ()
  {
    Echo ("BellePoule> ");
  }

  // --------------------------------------------------------------------------------
  gboolean Console::OnSocketEvent (GSocket      *socket,
                                   GIOCondition  condition,
                                   Console       *console)
  {
    GError  *error  = nullptr;
    gchar    buffer[512];
    guint    received;

    received = g_socket_receive (socket,
                                 buffer,
                                 sizeof (buffer),
                                 nullptr,
                                 &error);

    if (error)
    {
      g_warning ("g_socket_receive: %s", error->message);
      g_clear_error (&error);
    }

    if (received <= 0)
    {
      console->_listener->OnShellClosed (console);
    }
    else
    {
      for (size_t i = 0; i < received; i++)
      {
        gchar c = buffer[i];

        if (c == '\n')
        {
          console->_listener->OnCommand (console,
                                         console->_command->str);

          g_string_free (console->_command,
                         TRUE);
          console->_command = g_string_new ("");
          console->EchoPrompt ();
        }
        else
        {
          g_string_append_c (console->_command,
                             c);
        }
      }
    }

    return G_SOURCE_CONTINUE;
  }
}
