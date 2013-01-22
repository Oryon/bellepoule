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

#ifndef WIN32
#include <stdint.h>
#include <sys/socket.h>
#endif
#include <glib.h>
#include <microhttpd.h>

#include "util/object.hpp"

namespace Net
{
  class HttpServer : public Object
  {
    public:
      typedef gchar *(*GetHttpResponseCbk) (Object      *client,
                                            const gchar *url);

      HttpServer (Object            *client,
                  GetHttpResponseCbk get_http_response);

    private:
      static const guint PORT = 8080;

      struct MHD_Daemon  *_deamon;
      Object             *_client;
      GetHttpResponseCbk  _get_http_response;

      virtual ~HttpServer ();

      int OnGet (struct MHD_Connection *connection,
                 const char            *url,
                 const char            *method,
                 void                  **con_cls);
      static int OnMicroHttpRequest (HttpServer            *server,
                                     struct MHD_Connection *connection,
                                     const char            *url,
                                     const char            *method,
                                     const char            *version,
                                     const char            *upload_data,
                                     size_t                *upload_data_size,
                                     void                  **con_cls);
  };
}

#endif
