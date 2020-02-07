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

#include <libwebsockets.h>
#include <glib.h>

#include "util/object.hpp"
#include "server.hpp"

namespace Net
{
  class WebAppServer : public Server
  {
    public:
      WebAppServer (Listener *listener,
                    guint     channel,
                    guint     port);

      void SetIpV4Address (const gchar *ip_address);

      void SendMessageTo (guint    to,
                          Message *response);

    private:
      static guint _connection_count;

      struct OutgoingMessage
      {
        OutgoingMessage (guint    client,
                         Message *message);

        OutgoingMessage (guint        client,
                         const gchar *message);

        OutgoingMessage *Duplicate ();

        ~OutgoingMessage ();

        guint  _client_id;
        gchar *_message;
      };

      struct WebApp
      {
        struct lws      *_wsi;
        guint            _id;
        guint            _screen_id;
        RequestBody     *_input_buffer;
        OutgoingMessage *_pending_message;
      };

      struct Client
      {
        WebAppServer *_server;
        guint         _client_id;
        guint         _screen_id;
      };

      struct IncomingRequest : public RequestBody
      {
        IncomingRequest (Server *server,
                         guint   id);

        ~IncomingRequest () override;

        guint _id;
      };

    private:
      const gchar                      *_ip_address;
      guint                             _channel;
      gboolean                          _running;
      GThread                          *_thread;
      GAsyncQueue                      *_queue;
      struct lws_context               *_context;
      GList                            *_clients;
      struct lws_context_creation_info *_info;

      ~WebAppServer () override;

      static gboolean OnRequest (IncomingRequest *request);

      static gboolean OnNewClient (Client *client);

      static gpointer ThreadFunction (WebAppServer *server);

      static int OnHttp (struct lws                *wsi,
                         enum lws_callback_reasons  reason,
                         void                      *user,
                         gchar                     *in,
                         size_t                     len);

      static int OnSmartPouleData (struct lws                *wsi,
                                   enum lws_callback_reasons  reason,
                                   WebApp                    *client,
                                   char                      *in,
                                   size_t                     len);
  };
}
