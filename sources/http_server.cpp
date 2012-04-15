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
HttpServer::HttpServer ()
{
  _deamon = MHD_start_daemon (MHD_USE_DEBUG | MHD_USE_SELECT_INTERNALLY,
                              PORT,
                              NULL,
                              NULL,
                              (MHD_AccessHandlerCallback) OnClientRequest,
                              this,
                              MHD_OPTION_END);
}

// --------------------------------------------------------------------------------
HttpServer::~HttpServer ()
{
  MHD_stop_daemon (_deamon);
}

// --------------------------------------------------------------------------------
int HttpServer::OnClientRequest (HttpServer            *server,
                                 struct MHD_Connection *connection,
                                 const char            *url,
                                 const char            *method,
                                 const char            *version,
                                 const char            *upload_data,
                                 size_t                *upload_data_size,
                                 void                  **con_cls)
{
  int ret;

  if (url)     g_print ("url     ==> %s\n", url);
  if (method)  g_print ("method  ==> %s\n", method);
  if (version) g_print ("version ==> %s\n", version);

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
    return MHD_NO; // upload data in a GET!?
  }

  *con_cls = NULL;
  {
    struct MHD_Response *response;
    static char *page = "<html><head><title>libmicrohttpd demo</title>"\
                         "</head><body>libmicrohttpd demo</body></html>";

    response = MHD_create_response_from_data (strlen (page),
                                              (void *) page,
                                              MHD_NO,
                                              MHD_NO);
    ret = MHD_queue_response (connection,
                              MHD_HTTP_OK,
                              response);

    MHD_destroy_response (response);
  }

  return ret;
}
