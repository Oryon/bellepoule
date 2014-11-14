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

#include "web_server.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  WebServer::WebServer (StateFunc  state_func,
                        Object    *owner)
    : Object ("WebServer")
  {
#ifndef WIN32
    g_mutex_init (&_mutex);
    _thread = NULL;

    _in_progress  = FALSE;
    _on           = FALSE;
    _failed       = FALSE;
    _state_func   = state_func;
    _owner        = owner;
#endif
  }

  // --------------------------------------------------------------------------------
  WebServer::~WebServer ()
  {
#ifndef WIN32
    g_mutex_lock (&_mutex);
    ShutDown (this);

    if (_thread)
    {
      g_object_unref (_thread);
    }

    g_mutex_clear (&_mutex);
#endif
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
#ifndef WIN32
    if (g_mutex_trylock (&_mutex))
    {
      _thread = g_thread_new (NULL,
                              (GThreadFunc) StartUp,
                              this);
    }
#endif
  }

  // --------------------------------------------------------------------------------
  void WebServer::Stop ()
  {
#ifndef WIN32
    if (g_mutex_trylock (&_mutex))
    {
      _thread = g_thread_new (NULL,
                              (GThreadFunc) ShutDown,
                              this);
    }
#endif
  }

  // --------------------------------------------------------------------------------
  void WebServer::Spawn (const gchar *script)
  {
#ifndef WIN32
    if (_failed == FALSE)
    {
      GError *error       = NULL;
      gint    exit_status;
      gchar  *path        = g_build_filename (_program_path, "scripts", script, NULL);
      gchar  *cmd_line    = g_strdup_printf ("gksudo --preserve-env --description VideoServer %s", path);

      if (g_spawn_command_line_sync (cmd_line,
                                     NULL,
                                     NULL,
                                     &exit_status,
                                     &error) == FALSE)
      {
        if (error)
        {
          g_warning ("%s: %s", cmd_line, error->message);
          g_error_free (error);
        }
        _failed = TRUE;
      }
      else if (exit_status != 0)
      {
        _failed = TRUE;
      }

      g_free (cmd_line);
      g_free (path);
    }

    g_idle_add ((GSourceFunc) OnProgress,
                this);
#endif
  }

  // --------------------------------------------------------------------------------
  gpointer WebServer::StartUp (WebServer *server)
  {
#ifndef WIN32
    if (server->_on == FALSE)
    {
      server->Prepare ();
      server->Spawn ("wwwstart");

      server->_on          = (server->_failed == FALSE);
      server->_in_progress = FALSE;

      if (server->_thread)
      {
        g_thread_unref (server->_thread);
        server->_thread = NULL;
      }
    }
    g_mutex_unlock (&server->_mutex);
#endif

    return NULL;
  }

  // --------------------------------------------------------------------------------
  gpointer WebServer::ShutDown (WebServer *server)
  {
#ifndef WIN32
    if (server->_on)
    {
      server->Prepare ();
      server->Spawn ("wwwstop");

      server->_on          = (server->_failed == TRUE);
      server->_in_progress = FALSE;

      if (server->_thread)
      {
        g_thread_unref (server->_thread);
        server->_thread = NULL;
      }
    }
    g_mutex_unlock (&server->_mutex);
#endif

    return NULL;
  }

  // --------------------------------------------------------------------------------
  guint WebServer::OnProgress (WebServer *server)
  {
#ifndef WIN32
    server->_state_func (server->_in_progress,
                         server->_on,
                         server->_owner);

    return G_SOURCE_REMOVE;
#else
    return FALSE;
#endif
  }
}
