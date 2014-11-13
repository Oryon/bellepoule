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

#ifndef web_server_hpp
#define web_server_hpp

#include <glib.h>

#include "util/object.hpp"

namespace Net
{
  class WebServer : public Object
  {
    public:
      typedef void (*ProgressFunc) (gdouble  percent,
                                    Object  *owner);

      WebServer (ProgressFunc  progress_func,
                 Object       *owner);

      void Start ();

      void Stop ();

    private:
      GThread      *_thread;
      GMutex        _mutex;
      ProgressFunc  _progress_func;
      Object       *_owner;
      gdouble       _progress;
      gdouble       _step;

      virtual ~WebServer ();

      void Spawn (const gchar *cmd_line);

      static gpointer StartUp (WebServer *server);

      static gpointer ShutDown (WebServer *server);

      static guint OnProgress (WebServer *owner);
  };
}

#endif
