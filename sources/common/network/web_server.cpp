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
  WebServer::WebServer (ProgressFunc  progress_func,
                        Object       *owner)
    : Object ("WebServer")
  {
    g_mutex_init (&_mutex);
    _thread = NULL;

    _progress      = 0.0;
    _progress_func = progress_func;
    _owner         = owner;
  }

  // --------------------------------------------------------------------------------
  WebServer::~WebServer ()
  {
    ShutDown (this);

    if (_thread)
    {
      g_object_unref (_thread);
    }

    g_mutex_clear (&_mutex);
  }

  // --------------------------------------------------------------------------------
  void WebServer::Start ()
  {
    if (g_mutex_trylock (&_mutex))
    {
      _thread = g_thread_new (NULL,
                              (GThreadFunc) StartUp,
                              this);
      g_mutex_unlock (&_mutex);
    }
  }

  // --------------------------------------------------------------------------------
  void WebServer::Stop ()
  {
    if (g_mutex_trylock (&_mutex))
    {
      _thread = g_thread_new (NULL,
                              (GThreadFunc) ShutDown,
                              this);
      g_mutex_unlock (&_mutex);
    }
  }

  // --------------------------------------------------------------------------------
  void WebServer::Spawn (const gchar *cmd_line)
  {
    GError *error;

    if (g_spawn_command_line_sync (cmd_line,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &error) == FALSE)
    {
      if (error)
      {
        g_warning ("%s: %s", cmd_line, error->message);
        g_error_free (error);
      }
    }

    _progress += _step;
    g_idle_add ((GSourceFunc) OnProgress,
                this);
  }

  // --------------------------------------------------------------------------------
  gpointer WebServer::StartUp (WebServer *server)
  {
    g_mutex_lock (&server->_mutex);
    server->_step = 0.25;
    server->Spawn ("gksudo lighty-enable-mod fastcgi");
    server->Spawn ("gksudo lighty-enable-mod fastcgi-php");
    server->Spawn ("gksudo lighty-enable-mod bellepoule");
    server->Spawn ("gksudo /etc/init.d/lighttpd restart");

    g_thread_unref (server->_thread);
    server->_thread = NULL;
    g_mutex_unlock (&server->_mutex);

    return NULL;
  }

  // --------------------------------------------------------------------------------
  gpointer WebServer::ShutDown (WebServer *server)
  {
    g_mutex_lock (&server->_mutex);
    server->_step = -0.5;
    server->Spawn ("gksu lighty-disable-mod bellepoule");
    server->Spawn ("gksudo /etc/init.d/lighttpd stop");

    g_thread_unref (server->_thread);
    server->_thread = NULL;
    g_mutex_unlock (&server->_mutex);

    return NULL;
  }

  // --------------------------------------------------------------------------------
  guint WebServer::OnProgress (WebServer *server)
  {
    server->_progress_func (server->_progress,
                            server->_owner);

    return G_SOURCE_REMOVE;
  }
}
