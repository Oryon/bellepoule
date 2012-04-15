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

#ifndef http_server_hpp
#define http_server_hpp

#include <glib.h>
#include <microhttpd.h>

#include "object.hpp"

class HttpServer : public Object
{
  public:
    HttpServer ();

  private:
    static const guint PORT = 8080;

    struct MHD_Daemon *_deamon;

    ~HttpServer ();

    static int OnClientRequest (HttpServer            *server,
                                struct MHD_Connection *connection,
                                const char            *url,
                                const char            *method,
                                const char            *version,
                                const char            *upload_data,
                                size_t                *upload_data_size,
                                void                  **con_cls);
};

#endif
