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

#include "util/global.hpp"
#include "ring.hpp"

#include "web_server.hpp"

#ifdef G_OS_WIN32
#include "windows.h"
#endif

namespace Net
{
  // --------------------------------------------------------------------------------
  WebServer::WebServer (Listener *listener)
    : Object ("WebServer")
  {
     _listener  = listener;
     _child_pid = 0;
  }

  // --------------------------------------------------------------------------------
  WebServer::~WebServer ()
  {
    Stop ();
  }

  // --------------------------------------------------------------------------------
  void WebServer::Start ()
  {
    {
      gchar *cmd;

#ifdef G_OS_WIN32
      cmd = g_build_filename (Global::_share_dir, "scripts", "wwwstart.bat", NULL);
#else
      cmd = g_strdup ("killall php");
#endif

      Execute (cmd);
      g_free (cmd);
    }

#ifndef G_OS_WIN32
    {
      gchar   *working_dir = g_build_filename (Global::_www, "..", nullptr);
      gchar   *cmd_line    = g_strdup_printf ("php7.0 -S %s%s", Ring::_broker->GetIpV4Address (), Global::_www_port);
      GError  *error       = nullptr;
      gchar  **argvp;

      g_shell_parse_argv (cmd_line,
                          nullptr,
                          &argvp,
                          nullptr);

      if (g_spawn_async (working_dir,
                         argvp,
                         nullptr,
                         (GSpawnFlags) (G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH),
                         nullptr,
                         nullptr,
                         &_child_pid,
                         &error) == FALSE)
      {
        g_warning ("g_spawn_async: %s\n", error->message);
        g_clear_error (&error);

        _listener->OnWebServerState (FALSE,
                                     FALSE);
      }
      else
      {
        g_child_watch_add (_child_pid,
                           (GChildWatchFunc) OnChildStatus,
                           this);
        _listener->OnWebServerState (FALSE,
                                     TRUE);
      }

      g_strfreev (argvp);
      g_free (cmd_line);
      g_free (working_dir);
    }
#endif
  }

  // --------------------------------------------------------------------------------
  void WebServer::Stop ()
  {
#ifdef G_OS_WIN32
    {
      gchar *cmd = g_build_filename (Global::_share_dir, "scripts", "wwwstop.bat", NULL);

      Execute (cmd);
      g_free (cmd);
    }
#else
    if (_child_pid)
    {
      kill (_child_pid, SIGINT);
    }
#endif
  }

  // --------------------------------------------------------------------------------
  void WebServer::Execute (const gchar *cmd)
  {
    GError   *error        = nullptr;
    gint      exit_status;
    gboolean  spawn_status;

#ifdef G_OS_WIN32
    {
      guint windows_result;

      windows_result = (guint32) ShellExecute (NULL,
                                               "open",
                                               cmd,
                                               NULL,
                                               Global::_share_dir,
                                               SW_HIDE);
      spawn_status = windows_result > 32;
      exit_status  = 0;
    }
#else
    {
      spawn_status = g_spawn_command_line_sync (cmd,
                                                nullptr,
                                                nullptr,
                                                &exit_status,
                                                &error);
    }
#endif

    if (spawn_status == FALSE)
    {
      g_warning ("g_spawn_command_line_sync failed");
    }
    if (error)
    {
      g_warning ("%s error: %s", cmd, error->message);
      g_error_free (error);
    }
    if (exit_status == -1)
    {
      g_warning ("%s status: %d", cmd, exit_status);
    }
  }

  // --------------------------------------------------------------------------------
  void WebServer::OnChildStatus (GPid       pid,
                                 gint       status,
                                 WebServer *server)
  {
    g_spawn_close_pid (pid);
    server->_listener->OnWebServerState (FALSE,
                                         FALSE);
  }
}
