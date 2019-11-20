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

#include "util/global.hpp"
#include "message.hpp"
#include "webapp_server.hpp"

namespace Net
{
  guint WebAppServer::_connection_count = 0;

  // --------------------------------------------------------------------------------
  // --------------------------------------------------------------------------------
  WebAppServer::IncomingRequest::IncomingRequest (Server *server,
                                                  guint   id)
    : RequestBody (server)
  {
    _id = id;
  }

  // --------------------------------------------------------------------------------
  WebAppServer::IncomingRequest::~IncomingRequest ()
  {
  }

  // --------------------------------------------------------------------------------
  // --------------------------------------------------------------------------------
  WebAppServer::OutgoingMessage::OutgoingMessage (guint    client,
                                                  Message *message)
  {
    _client_id = client;
    _message   = message->GetParcel ();
  }

  // --------------------------------------------------------------------------------
  WebAppServer::OutgoingMessage::~OutgoingMessage ()
  {
    g_free (_message);
  }

  // --------------------------------------------------------------------------------
  void WebAppServer::OutgoingMessage::Free (OutgoingMessage *message)
  {
    delete (message);
  }

  // --------------------------------------------------------------------------------
  // --------------------------------------------------------------------------------
  WebAppServer::WebAppServer (Listener *listener,
                              guint     port)
    : Server (listener,
              port)
  {
    _running = TRUE;
    _clients = nullptr;

    _queue = g_async_queue_new_full ((GDestroyNotify) Object::TryToRelease);

    {
      struct lws_protocols *protocols;

      {
        protocols = g_new0 (struct lws_protocols, 3);

        protocols[0].name                  = "http-only";
        protocols[0].callback              = WebAppServer::OnHttp;
        protocols[0].per_session_data_size = 0;
        protocols[0].rx_buffer_size        = 0;
        protocols[0].id                    = 0;
        protocols[0].user                  = this;

        protocols[1].name                  = "smartpoule";
        protocols[1].callback              = (lws_callback_function *) OnSmartPouleData;
        protocols[1].per_session_data_size = sizeof (WebApp);
        protocols[1].rx_buffer_size        = BUFFER_SIZE;
        protocols[1].id                    = 0;
        protocols[1].user                  = this;
      }

      _info = g_new0 (struct lws_context_creation_info, 1);
      _info->port      = port;
      _info->protocols = protocols;
      _info->gid       = -1;
      _info->uid       = -1;

      _context = lws_create_context (_info);
    }

    {
      GError *error = nullptr;

      _thread = g_thread_try_new ("WebAppServer",
                                  (GThreadFunc) ThreadFunction,
                                  this,
                                  &error);
      if (_thread == nullptr)
      {
        g_printerr ("Failed to create Uploader thread: %s\n", error->message);
        g_error_free (error);

        Release ();
      }
    }
  }

  // --------------------------------------------------------------------------------
  WebAppServer::~WebAppServer ()
  {
    printf (MAGENTA "*** WebAppServer ***\n" ESC);

    _running = FALSE;

    if (_thread)
    {
      lws_cancel_service (_context);
      g_thread_join (_thread);
    }

    lws_context_destroy (_context);
    g_free ((void *) _info->protocols);
    g_free (_info);

    g_async_queue_unref (_queue);
  }

  // --------------------------------------------------------------------------------
  void WebAppServer::SendBackResponse (Message *question,
                                       Message *response)
  {
    OutgoingMessage *outgoing_message = new OutgoingMessage (question->GetInteger ("client"),
                                                             response);

    g_async_queue_push (_queue,
                        outgoing_message);

    lws_cancel_service (_context);
  }

  // --------------------------------------------------------------------------------
  gboolean WebAppServer::OnScoreSheetCall (IncomingRequest *request)
  {
    WebAppServer *server  = dynamic_cast<WebAppServer *> (request->_server);
    Message      *message = new Message ("SmartPoule::ScoreSheetCall");

    {
      gsize   size;
      guchar *data = g_base64_decode (&request->_data[1],
                                      &size);

      if (data)
      {
        gchar **tokens = g_strsplit_set ((gchar *) data,
                                         "#/.",
                                         0);

        if (tokens && tokens[0] && tokens[1] && tokens[2] && tokens[3] && tokens[4])
        {
          message->Set ("client",      request->_id);
          message->Set ("competition", (guint) g_ascii_strtoll (tokens[1], nullptr, 16));
          message->Set ("stage",       (guint) g_ascii_strtoll (tokens[2], nullptr, 16));
          message->Set ("batch",       tokens[3]);
          message->Set ("bout",        tokens[4]);

          message->Dump (TRUE);
          server->_listener->OnMessage (message);
        }
        g_strfreev (tokens);
        g_free (data);
      }
    }

    delete (request);
    return G_SOURCE_REMOVE;
  }

  // --------------------------------------------------------------------------------
  int WebAppServer::OnHttp (struct lws                *wsi,
                            enum lws_callback_reasons  reason,
                            void                      *user,
                            void                      *in,
                            size_t                     len)
  {
#pragma GCC diagnostic ignored "-Wswitch-enum"
    switch (reason)
    {
      case LWS_CALLBACK_HTTP:
      {
        gchar *app_path = g_build_filename (Global::_share_dir, "resources", "webapps", "smartpoule.html", nullptr);

        lws_serve_http_file (wsi,
                             app_path,
                             "text/html",
                             nullptr, 0);
        g_free (app_path);
      }
      break;

      default:
      break;
    }
#pragma GCC diagnostic pop

    return 0;
  }

  // --------------------------------------------------------------------------------
  int WebAppServer::OnSmartPouleData (struct lws                *wsi,
                                      enum lws_callback_reasons  reason,
                                      WebApp                    *web_app,
                                      char                      *in,
                                      size_t                     len)
  {
    WebAppServer *server = nullptr;

    if ((WebAppServer *) lws_get_protocol (wsi))
    {
      server = ((WebAppServer *) (lws_get_protocol (wsi)->user));
    }

#pragma GCC diagnostic ignored "-Wswitch-enum"
    switch (reason)
    {
      case LWS_CALLBACK_ESTABLISHED:
      {
        printf (GREEN "LWS_CALLBACK_ESTABLISHED %p/%p\n" ESC, (void *) web_app, (void *) wsi);

        web_app->_wsi              = wsi;
        web_app->_id               = ++_connection_count;
        web_app->_input_buffer     = nullptr;
        web_app->_pending_messages = nullptr;

        server->_clients = g_list_prepend (server->_clients,
                                           web_app);
      }
      break;

      case LWS_CALLBACK_CLOSED:
      {
        printf (RED "LWS_CALLBACK_CLOSED      %p/%p\n" ESC, (void *) web_app, (void *) wsi);

        delete (web_app->_input_buffer);
        g_list_free_full (web_app->_pending_messages,
                          (GDestroyNotify) OutgoingMessage::Free);

        server->_clients = g_list_remove (server->_clients,
                                          web_app);
      }
      break;

      case LWS_CALLBACK_RECEIVE:
      {
        if (web_app->_input_buffer == nullptr)
        {
          web_app->_input_buffer = new IncomingRequest (server,
                                                        web_app->_id);
        }

        web_app->_input_buffer->Append (in,
                                        len);

        if (lws_is_final_fragment (wsi))
        {
          web_app->_input_buffer->ZeroTerminate ();
          g_idle_add ((GSourceFunc) OnScoreSheetCall,
                      web_app->_input_buffer);
          web_app->_input_buffer = nullptr;
        }
      }
      break;

      case LWS_CALLBACK_SERVER_WRITEABLE:
      {
        while (web_app->_pending_messages)
        {
          OutgoingMessage *message = (OutgoingMessage *) web_app->_pending_messages->data;
          GString         *buffer  = g_string_new (nullptr);

          g_string_append_len (buffer,
                               "", LWS_PRE);

          g_string_append_printf (buffer,
                                  "%s", message->_message);

          {
            guchar *msg = (guchar *) &buffer->str[LWS_PRE];

            lws_write (wsi,
                       msg,
                       strlen ((gchar *) msg) + 1,
                       LWS_WRITE_TEXT);
          }

          g_string_free (buffer,
                         TRUE);

          delete (message);
          web_app->_pending_messages = g_list_remove_link (web_app->_pending_messages,
                                                           web_app->_pending_messages);
        }
      }
      break;

      default:
      break;
    }
#pragma GCC diagnostic pop

    return 0;
  }

  // --------------------------------------------------------------------------------
  gpointer WebAppServer::ThreadFunction (WebAppServer *server)
  {
    while (server->_running)
    {
      lws_service (server->_context, -1);

      {
        OutgoingMessage *outgoing_message = (OutgoingMessage *) g_async_queue_timeout_pop (server->_queue,
                                                                                           1000);
        if (outgoing_message)
        {
          gboolean retained = FALSE;

          for (GList *current = server->_clients; current != nullptr; current = g_list_next (current))
          {
            WebApp *web_app = (WebApp *) current->data;

            if (web_app->_id == outgoing_message->_client_id)
            {
              web_app->_pending_messages = g_list_append (web_app->_pending_messages,
                                                          outgoing_message);
              lws_callback_on_writable (web_app->_wsi);
              retained = TRUE;
              break;
            }
          }

          if (retained == FALSE)
          {
            delete outgoing_message;
          }
        }
      }
    }

    return nullptr;
  }
}
