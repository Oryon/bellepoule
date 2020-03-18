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
  class Pattern;

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
      struct Resource
      {
        const gchar *suffix;
        const gchar *folder;
        const gchar *mime_type;
      };

      struct OutgoingMessage
      {
        OutgoingMessage (guint    client_uuid,
                         Message *message);

        OutgoingMessage (guint        client_uuid,
                         const gchar *message);

        OutgoingMessage *Duplicate ();

        ~OutgoingMessage ();

        guint  _client_uuid;
        gchar *_message;
      };

      struct IncomingRequest : public RequestBody
      {
        IncomingRequest (Server *server,
                         guint   id);

        ~IncomingRequest () override;

        guint _client_uuid;
      };

      struct WebApp
      {
        struct lws      *_wsi;
        guint            _uuid;
        RequestBody     *_input_buffer;
        OutgoingMessage *_pending_message;
      };

      struct IdMap
      {
        guint _uuid;
        guint _alias;
      };

    private:
      static Resource _resources[];

      Pattern                          *_piste_pattern;
      Pattern                          *_referee_pattern;
      Pattern                          *_standalone_referee_pattern;
      const gchar                      *_ip_address;
      guint                             _channel;
      gboolean                          _running;
      GThread                          *_thread;
      GAsyncQueue                      *_queue;
      struct lws_context               *_context;
      GList                            *_clients;
      GList                            *_aliases;
      struct lws_context_creation_info *_info;

      ~WebAppServer () override;

      static gboolean OnIncomingMessage (IncomingRequest *request);

      static gboolean OnClientClosed (IncomingRequest *request);

      void OnNewMirror (guint uuid,
                        guint alias);

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
