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

#include "shell.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  Shell::Shell (GSocket  *socket,
                Listener *listener)
    : Object ("Shell")
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
      const gchar *logo = "\n"
        "                           .,,,,,,,,,,,,,,,'.         \n"
        "                          'kKXXNNNNNNXXXXX0o.         \n"
        "                          ,k000KXXXXXXXX0o'           \n"
        "                          ,O00000KXXXX0o'             \n"
        "                          ,OKK0KK0KKOo'               \n"
        "                          ,OKKKKKK0d;.                \n"
        "                          ,OKKKKKxc;lo,.              \n"
        "                          ,OXKKOxodkkkkd;.            \n"
        "                          ,OK0OkO00OOOOOOx;.          \n"
        "                          ,O000000000000000x:'.       \n"
        "                         'okO00KKKKKKKKKKK0000;       \n"
        "                       'oOOdoook0KK00000000OO0:       \n"
        "                     'lOKKOdoolclk000000OOOO00:       \n"
        "                   .lOK000kddddoccok000OOO0000c       \n"
        "                 .lk000000kxdddddoc:clxOO0OOOOc       \n"
        "               .cx00000000Oxxxxxxo;.  .:xOOOOO:       \n"
        "             .ckOOOOO00000Okkkkd:.      .:xOOOc       \n"
        "          .cdk000OOOOOO0OOOOxdc.          .:xOc       \n"
        "        .ck0000O00OOOOOOOOkl..              .'.       \n"
        "        .,,'',''''''''''''.                           \n"
        "\n"
        "mmmmm         \"\"#    \"\"#           mmmmm                \"\"#          \n"
        "#    #  mmm     #      #     mmm   #   \"#  mmm   m   m    #     mmm  \n"
        "#mmmm\" #\"  #    #      #    #\"  #  #mmm#\" #\" \"#  #   #    #    #\"  # \n"
        "#    # #\"\"\"\"    #      #    #\"\"\"\"  #      #   #  #   #    #    #\"\"\"\" \n"
        "#mmmm\" \"#mm\"    \"mm    \"mm  \"#mm\"  #      \"#m#\"  \"mm\"#    \"mm  \"#mm\" \n"
        "\n";

      Echo       (logo);
      EchoPrompt ();
    }
  }

  // --------------------------------------------------------------------------------
  Shell::~Shell ()
  {
    g_string_free (_command,
                   TRUE);

    g_source_destroy (_source);
    g_object_unref   (_socket);

    printf ("<<<  Released  >>>\n");
  }

  // --------------------------------------------------------------------------------
  void Shell::Echo (const gchar *message)
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
  void Shell::EchoPrompt ()
  {
    Echo ("BellePoule> ");
  }

  // --------------------------------------------------------------------------------
  gboolean Shell::OnSocketEvent (GSocket      *socket,
                                 GIOCondition  condition,
                                 Shell       *shell)
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
      shell->_listener->OnShellClosed (shell);
    }
    else
    {
      for (size_t i = 0; i < received; i++)
      {
        gchar c = buffer[i];

        if (c == '\n')
        {
          if (g_strcmp0 (shell->_command->str,
                         "help\r") == 0)
          {
            shell->Echo ("retained    dump a summary of the retained objects.\n");
          }
          else if (g_strcmp0 (shell->_command->str,
                              "retained\r") == 0)
          {
            Object::DumpList ();
          }

          g_string_free (shell->_command,
                         TRUE);
          shell->_command = g_string_new ("");
          shell->EchoPrompt ();
        }
        else
        {
          g_string_append_c (shell->_command,
                             c);
        }
      }
    }

    return G_SOURCE_CONTINUE;
  }
}
