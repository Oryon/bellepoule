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

#include "http_server.hpp"

// --------------------------------------------------------------------------------
HttpServer::HttpServer (Object             *client,
                        GetHttpResponseCbk  get_http_response)
: Object ("HttpServer")
{
  _deamon = MHD_start_daemon (MHD_USE_DEBUG | MHD_USE_SELECT_INTERNALLY,
                              PORT,
                              NULL,
                              NULL,
                              (MHD_AccessHandlerCallback) OnMicroHttpRequest,
                              this,
                              MHD_OPTION_END);
  _client            = client;
  _get_http_response = get_http_response;
}

// --------------------------------------------------------------------------------
HttpServer::~HttpServer ()
{
  MHD_stop_daemon (_deamon);
}

// --------------------------------------------------------------------------------
int HttpServer::OnGet (struct MHD_Connection *connection,
                       const char            *url,
                       const char            *method,
                       void                  **con_cls)
{
  int    ret             = MHD_NO;
  gchar *client_response = _get_http_response (_client,
                                               url);

  if (client_response)
  {
    struct MHD_Response *response;
    char                *page;

    page = g_strdup (client_response);
    g_free (client_response);

    *con_cls = NULL;

    response = MHD_create_response_from_data (strlen (page),
                                              (void *) page,
                                              MHD_YES,
                                              MHD_NO);
    ret = MHD_queue_response (connection,
                              MHD_HTTP_OK,
                              response);

    MHD_destroy_response (response);
  }

  return ret;
}

// --------------------------------------------------------------------------------
int HttpServer::OnMicroHttpRequest (HttpServer            *server,
                                    struct MHD_Connection *connection,
                                    const char            *url,
                                    const char            *method,
                                    const char            *version,
                                    const char            *upload_data,
                                    size_t                *upload_data_size,
                                    void                  **con_cls)
{
  //if (url)    g_print ("url     ==> %s\n", url);
  //if (method) g_print ("method  ==> %s\n", method);

  if (strcmp (method, "GET") != 0)
  {
    return MHD_NO;
  }

  if (server != *con_cls)
  {
    /* The first time only the headers are valid,
       do not respond in the first round... */
    *con_cls = server;
    return MHD_YES;
  }

  if (*upload_data_size != 0)
  {
    return MHD_NO;
  }

  return server->OnGet (connection,
                        url,
                        method,
                        con_cls);
}
