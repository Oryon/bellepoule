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

#ifdef WIN32
#include "windows.h"
#endif

#include "util/global.hpp"

#include "web_server.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  WebServer::WebServer (StateFunc  state_func,
                        Object    *owner)
    : Object ("WebServer")
  {
    g_mutex_init (&_mutex);

    _in_progress  = FALSE;
    _on           = FALSE;
    _failed       = FALSE;
    _state_func   = state_func;
    _owner        = owner;
  }

  // --------------------------------------------------------------------------------
  WebServer::~WebServer ()
  {
    g_mutex_lock (&_mutex);
    ShutDown (this);

    g_mutex_clear (&_mutex);
  }

  // --------------------------------------------------------------------------------
  void WebServer::Prepare ()
  {
    _failed      = FALSE;
    _in_progress = TRUE;

    g_idle_add ((GSourceFunc) OnProgress,
                this);
  }

  // --------------------------------------------------------------------------------
  void WebServer::Start ()
  {
    if (g_mutex_trylock (&_mutex) == FALSE)
    {
      g_thread_try_new ("WebServer::Start",
                        (GThreadFunc) StartUp,
                        this,
                        NULL);
    }
  }

  // --------------------------------------------------------------------------------
  void WebServer::Stop ()
  {
    if (g_mutex_trylock (&_mutex) == FALSE)
    {
      g_thread_try_new ("WebServer::Stop",
                        (GThreadFunc) ShutDown,
                        this,
                        NULL);
    }
  }

  // --------------------------------------------------------------------------------
  void WebServer::Spawn (const gchar *script)
  {
    if (_failed == FALSE)
    {
      GError   *error       = NULL;
      gint      exit_status;
      gchar    *path        = g_build_filename (Global::_share_dir, "scripts", script, NULL);
      gchar    *cmd_line;
      gboolean  spawn_status;

#ifdef WIN32
      {
        guint windows_result;

        cmd_line = g_strdup_printf ("%s.bat", path);

        windows_result = (guint32) ShellExecute (NULL,
                                                 "open",
                                                 cmd_line,
                                                 NULL,
                                                 Global::_share_dir,
                                                 SW_HIDE);
        spawn_status = windows_result > 32;
        exit_status  = 0;
      }
#else
      {
        cmd_line   = g_strdup_printf ("gksudo --preserve-env --description VideoServer %s.sh", path);
        spawn_status = g_spawn_command_line_sync (cmd_line,
                                                  NULL,
                                                  NULL,
                                                  &exit_status,
                                                  &error);
      }
#endif

      if (spawn_status == FALSE)
      {
        _failed = TRUE;
      }
      if (exit_status != 0)
      {
        g_warning ("%s status: %d", cmd_line, exit_status);
        _failed = TRUE;
      }
      if (error)
      {
        g_warning ("%s error: %s", cmd_line, error->message);
        g_error_free (error);
        _failed = TRUE;
      }

      g_free (cmd_line);
      g_free (path);
    }
  }

  // --------------------------------------------------------------------------------
  gpointer WebServer::StartUp (WebServer *server)
  {
    if (server->_on == FALSE)
    {
      server->Prepare ();
      server->Spawn ("wwwstart");

      server->_on          = (server->_failed == FALSE);
      server->_in_progress = FALSE;
    }
    g_mutex_unlock (&server->_mutex);

    g_idle_add ((GSourceFunc) OnProgress,
                server);

    return NULL;
  }

  // --------------------------------------------------------------------------------
  gpointer WebServer::ShutDown (WebServer *server)
  {
    if (server->_on)
    {
      server->Prepare ();
      server->Spawn ("wwwstop");

      server->_on          = (server->_failed == TRUE);
      server->_in_progress = FALSE;
    }
    g_mutex_unlock (&server->_mutex);

    g_idle_add ((GSourceFunc) OnProgress,
                server);

    return NULL;
  }

  // --------------------------------------------------------------------------------
  guint WebServer::OnProgress (WebServer *server)
  {
    server->_state_func (server->_in_progress,
                         server->_on,
                         server->_owner);

    return FALSE;
  }
}
