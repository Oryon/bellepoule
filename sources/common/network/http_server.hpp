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

#pragma once

#include <glib.h>
#include <microhttpd.h>

#include "util/object.hpp"
#include "cryptor.hpp"

namespace Net
{
  class HttpServer : public Object
  {
    public:
      class Client
      {
        public:
          Client () {};

          virtual gchar *GetSecretKey (const gchar *authentication_scheme) = 0;

        protected:
          virtual ~Client () {};
      };

    public:
      typedef gboolean (*HttpPost) (Client  *client,
                                    Message *message);
      typedef gchar *(*HttpGet) (Client      *client,
                                 const gchar *url);

      HttpServer (Client   *client,
                  HttpPost  http_post,
                  HttpGet   http_get,
                  guint     port);

      guint GetPort ();

    private:
      struct RequestBody
      {
        RequestBody (HttpServer *server);
        ~RequestBody ();

        void Append (const char *buf,
                     size_t      len);
        void Replace (const char *buf);
        void ZeroTerminate ();

        gchar      *_data;
        guint       _length;
        HttpServer *_server;
      };

      struct MHD_Daemon *_daemon;
      guint              _port;
      Client            *_client;
      HttpPost           _http_POST_cbk;
      HttpGet            _http_GET_cbk;
      Cryptor           *_cryptor;
      gchar             *_iv;

      virtual ~HttpServer ();

      static gboolean DeferedPost (RequestBody *request_body);

      int OnRequestReceived (struct MHD_Connection *connection,
                             RequestBody           *request_body,
                             const char            *url,
                             const char            *method,
                             const char            *upload_data,
                             size_t                *upload_data_size);

      static int HeaderIterator (HttpServer         *server,
                                 enum MHD_ValueKind  kind,
                                 const char         *key,
                                 const char         *value);

      static int OnMicroHttpRequest (HttpServer            *server,
                                     struct MHD_Connection *connection,
                                     const char            *url,
                                     const char            *method,
                                     const char            *version,
                                     const char            *upload_data,
                                     size_t                *upload_data_size,
                                     RequestBody           **request_body);

      static void OnMicroHttpRequestCompleted (HttpServer                      *server,
                                               struct MHD_Connection           *connection,
                                               RequestBody                     **request_body,
                                               enum MHD_RequestTerminationCode   code);
  };
}
