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

#include <sys/types.h>

#include "cryptor.hpp"
#include "message.hpp"
#include "http_server.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  HttpServer::RequestBody::RequestBody (HttpServer *server)
  {
    _server = server;
    _data   = NULL;
    _length = 0;
  }

  // --------------------------------------------------------------------------------
  HttpServer::RequestBody::~RequestBody ()
  {
    g_free (_data);
  }

  // --------------------------------------------------------------------------------
  void HttpServer::RequestBody::ZeroTerminate ()
  {
    _data = (gchar *) g_realloc (_data,
                                 _length + 1);
    _data[_length] = '\0';
  }

  // --------------------------------------------------------------------------------
  void HttpServer::RequestBody::Append (const char *buf,
                                        size_t      len)
  {
    _data = (gchar *) g_realloc (_data,
                                 _length + len);
    strncpy (&_data[_length],
             buf,
             len);
    _length += len;
  }

  // --------------------------------------------------------------------------------
  void HttpServer::RequestBody::Replace (const char *buf)
  {
    g_free (_data);
    _data   = NULL;
    _length = 0;

    if (buf)
    {
      Append (buf,
              strlen (buf) + 1);
    }
  }
}

namespace Net
{
  // --------------------------------------------------------------------------------
  HttpServer::HttpServer (Client   *client,
                          HttpPost  http_post,
                          HttpGet   http_get,
                          guint     port)
    : Object ("HttpServer")
  {
    _port = port;

    _cryptor = new Cryptor ();

    _client        = client;
    _http_POST_cbk = http_post;
    _http_GET_cbk  = http_get;

    _iv = NULL;

    _daemon = MHD_start_daemon (MHD_USE_DEBUG | MHD_USE_SELECT_INTERNALLY,
                                port,
                                NULL, NULL,
                                (MHD_AccessHandlerCallback)  OnMicroHttpRequest,          this,
                                MHD_OPTION_NOTIFY_COMPLETED, OnMicroHttpRequestCompleted, this,
                                MHD_OPTION_END);
  }

  // --------------------------------------------------------------------------------
  HttpServer::~HttpServer ()
  {
    MHD_stop_daemon (_daemon);

    _cryptor->Release ();
    g_free (_iv);
  }

  // --------------------------------------------------------------------------------
  guint HttpServer::GetPort ()
  {
    return _port;
  }

  // --------------------------------------------------------------------------------
  gboolean HttpServer::DeferedPost (RequestBody *request_body)
  {
    Message *message;

    {
      gchar *content = g_strndup (request_body->_data,
                                  request_body->_length);

      message = new Message ((const guint8 *) content);

      if (message->IsValid () == FALSE)
      {
        message->Set ("content",
                      content);
      }

      g_free (content);
    }

    request_body->_server->_http_POST_cbk (request_body->_server->_client,
                                           message);

    message->Release ();
    delete (request_body);

    return G_SOURCE_REMOVE;
  }

  // --------------------------------------------------------------------------------
  int HttpServer::HeaderIterator (HttpServer         *server,
                                  enum MHD_ValueKind  kind,
                                  const char         *key,
                                  const char         *value)
  {
    if (g_strcmp0 (key, "IV") == 0)
    {
      server->_iv = g_strdup (value);
      return MHD_NO;
    }
    return MHD_YES;
  }

  // --------------------------------------------------------------------------------
  int HttpServer::OnRequestReceived (struct MHD_Connection *connection,
                                     RequestBody           *request_body,
                                     const char            *url,
                                     const char            *method,
                                     const char            *upload_data,
                                     size_t                *upload_data_size)
  {
    int ret = MHD_NO;

    if (_http_GET_cbk && (g_strcmp0 (method, "GET") == 0))
    {
      if (*upload_data_size == 0)
      {
        gchar *client_response = _http_GET_cbk (_client,
                                                url);

        if (client_response)
        {
          struct MHD_Response *response;

          response = MHD_create_response_from_buffer (strlen (client_response),
                                                      (void *) client_response,
                                                      MHD_RESPMEM_MUST_COPY);
          ret = MHD_queue_response (connection,
                                    MHD_HTTP_OK,
                                    response);

          MHD_destroy_response (response);
          g_free (client_response);
        }
      }
    }
    else if (_http_POST_cbk && ((g_strcmp0 (method, "POST") == 0) || (g_strcmp0 (method, "PUT") == 0)))
    {
      if (*upload_data_size)
      {
        request_body->Append (upload_data,
                              *upload_data_size);
        *upload_data_size = 0;
        return MHD_YES;
      }
      else
      {
        {
          g_free (_iv);
          _iv = NULL;

          MHD_get_connection_values (connection,
                                     MHD_HEADER_KIND,
                                     (MHD_KeyValueIterator) HeaderIterator,
                                     this);
        }

        if (g_strrstr (request_body->_data,
                       "[Header]") == NULL)
        {
          gchar *key = _client->GetSecretKey (url);

          if (key)
          {
            gchar *decrypted;

            request_body->ZeroTerminate ();
            decrypted = _cryptor->Decrypt (request_body->_data,
                                           _iv,
                                           key);
            request_body->Replace (decrypted);

            g_free (decrypted);
            g_free (key);
          }
        }

        {
          struct MHD_Response *response;

          response = MHD_create_response_from_buffer (0,
                                                      NULL,
                                                      MHD_RESPMEM_PERSISTENT);
          ret = MHD_queue_response (connection,
                                    MHD_HTTP_OK,
                                    response);

          MHD_destroy_response (response);
        }
      }
    }

    return ret;
  }

  // --------------------------------------------------------------------------------
  void HttpServer::OnMicroHttpRequestCompleted (HttpServer                      *server,
                                                struct MHD_Connection           *connection,
                                                RequestBody                     **request_body,
                                                enum MHD_RequestTerminationCode   code)
  {
    g_free (server->_iv);
    server->_iv = NULL;

    if (*request_body)
    {
      g_idle_add ((GSourceFunc) DeferedPost,
                  *request_body);
      *request_body = NULL;
    }
  }

  // --------------------------------------------------------------------------------
  int HttpServer::OnMicroHttpRequest (HttpServer            *server,
                                      struct MHD_Connection *connection,
                                      const char            *url,
                                      const char            *method,
                                      const char            *version,
                                      const char            *upload_data,
                                      size_t                *upload_data_size,
                                      RequestBody           **request_body)
  {
    if (*request_body == NULL)
    {
      *request_body = new RequestBody (server);
      return MHD_YES;
    }

    return server->OnRequestReceived (connection,
                                      *request_body,
                                      url,
                                      method,
                                      upload_data,
                                      upload_data_size);
  }
}
