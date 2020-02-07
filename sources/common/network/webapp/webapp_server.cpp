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
#include "util/flash_code.hpp"
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
  WebAppServer::OutgoingMessage::OutgoingMessage (guint        client,
                                                  const gchar *message)
  {
    _client_id = client;
    _message   = g_strdup (message);
  }

  // --------------------------------------------------------------------------------
  WebAppServer::OutgoingMessage::~OutgoingMessage ()
  {
    g_free (_message);
  }

  // --------------------------------------------------------------------------------
  WebAppServer::OutgoingMessage *WebAppServer::OutgoingMessage::Duplicate ()
  {
    return new OutgoingMessage (_client_id,
                                _message);
  }

  // --------------------------------------------------------------------------------
  // --------------------------------------------------------------------------------
  WebAppServer::WebAppServer (Listener *listener,
                              guint     channel,
                              guint     port)
    : Server (listener,
              port)
  {
    _running    = TRUE;
    _clients    = nullptr;
    _channel    = channel;
    _ip_address = nullptr;

    _queue = g_async_queue_new_full ((GDestroyNotify) Object::TryToRelease);

    {
      struct lws_protocols *protocols;

      {
        protocols = g_new0 (struct lws_protocols, 3);

        protocols[0].name                  = "http-only";
        protocols[0].callback              = (lws_callback_function *) WebAppServer::OnHttp;
        protocols[0].per_session_data_size = 0;
        protocols[0].rx_buffer_size        = 0;
        protocols[0].id                    = 0;
        protocols[0].user                  = this;

        protocols[1].name                  = "smartpoule";
        protocols[1].callback              = (lws_callback_function *) OnSmartPouleData;
        protocols[1].per_session_data_size = sizeof (WebApp);
        protocols[1].rx_buffer_size        = 0;
        protocols[1].id                    = 0;
        protocols[1].user                  = this;
      }

      _info = g_new0 (struct lws_context_creation_info, 1);
      _info->port      = port;
      _info->protocols = protocols;
      _info->gid       = -1;
      _info->uid       = -1;
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
    _running = FALSE;

    if (_thread)
    {
      if (_context)
      {
        lws_cancel_service (_context);
      }
      g_thread_join (_thread);
    }

    printf (MAGENTA "*** WebAppServer ***\n" ESC);

    g_free ((void *) _info->protocols);
    g_free (_info);

    g_async_queue_unref (_queue);
  }

  // --------------------------------------------------------------------------------
  void WebAppServer::SetIpV4Address (const gchar *ip_address)
  {
    _ip_address = ip_address;
  }

  // --------------------------------------------------------------------------------
  void WebAppServer::SendMessageTo (guint    to,
                                    Message *response)
  {
    OutgoingMessage *outgoing_message = new OutgoingMessage (to,
                                                             response);

    g_async_queue_push (_queue,
                        outgoing_message);

    lws_cancel_service (_context);
  }

  // --------------------------------------------------------------------------------
  gboolean WebAppServer::OnRequest (IncomingRequest *request)
  {
    WebAppServer *server  = dynamic_cast<WebAppServer *> (request->_server);
    Message      *message = new Message ((const guint8 *) request->_data);

    message->Set ("client",  request->_id);
    message->Set ("channel", server->_channel);
    server->_listener->OnMessage (message);

    message->Release ();
    delete (request);
    return G_SOURCE_REMOVE;
  }

  // --------------------------------------------------------------------------------
  gboolean WebAppServer::OnNewClient (Client *client)
  {
    WebAppServer *server = dynamic_cast<WebAppServer *> (client->_server);
    gchar        *png64;

    {
      GdkPixbuf *pixbuf;
      gchar     *png;
      gsize      png_size;
      GError    *error    = nullptr;

      {
        gchar *code = g_strdup_printf ("%s:%d/arbitre/arene/%d",
                                       server->_ip_address,
                                       server->GetPort (),
                                       client->_screen_id);

        pixbuf = FlashCode::GetPixbuf (code);

        g_free (code);
      }

      if (gdk_pixbuf_save_to_buffer (pixbuf,
                                     &png,
                                     &png_size,
                                     "png",
                                     &error,
                                     nullptr) == FALSE)
      {
        g_printerr ("Failed to create Uploader thread: %s\n", error->message);
        g_error_free (error);
      }

      png64 = g_base64_encode ((guchar *) png,
                               png_size);

      g_free (png);
      g_object_unref (pixbuf);
    }

    {
      Message *message = new Message ("BellePoule::QrCode");

      message->Set ("qrcode.png", png64);

      server->SendMessageTo (client->_client_id,
                             message);
    }

    g_free (png64);

    delete (client);
    return G_SOURCE_REMOVE;
  }

  // --------------------------------------------------------------------------------
  int WebAppServer::OnHttp (struct lws                *wsi,
                            enum lws_callback_reasons  reason,
                            void                      *user,
                            gchar                     *in,
                            size_t                     len)
  {
#pragma GCC diagnostic ignored "-Wswitch-enum"
    switch (reason)
    {
      case LWS_CALLBACK_HTTP:
      {
        if (g_str_has_suffix (in, ".png"))
        {
          gchar *base_name = g_path_get_basename (in);
          gchar *app_path  = g_build_filename (Global::_share_dir, "resources", "webapps", base_name, nullptr);

          lws_serve_http_file (wsi,
                               app_path,
                               "image/svg",
                               nullptr, 0);
          g_free (app_path);
          g_free (base_name);
        }
        else
        {
          gchar *app_path = g_build_filename (Global::_share_dir, "resources", "webapps", "smartpoule.html", nullptr);

          lws_serve_http_file (wsi,
                               app_path,
                               "text/html",
                               nullptr, 0);
          g_free (app_path);
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

        web_app->_wsi             = wsi;
        web_app->_id              = ++_connection_count;
        web_app->_input_buffer    = nullptr;
        web_app->_pending_message = nullptr;

        {
          char peer_name[128];
          char ip[30];

          lws_get_peer_addresses (wsi,
                                  lws_get_socket_fd (wsi),
                                  peer_name, sizeof peer_name,
                                  ip, sizeof ip);

          {
            gchar **splitted_ip = g_strsplit_set (ip,
                                                  ".",
                                                  0);

            if (splitted_ip && splitted_ip[0] && splitted_ip[1] && splitted_ip[2] && splitted_ip[3])
            {
              Client *client;

              web_app->_screen_id = g_ascii_strtoull (splitted_ip[3],
                                                      nullptr,
                                                      10);
              client = new Client ();
              client->_server    = server;
              client->_client_id = web_app->_id;
              client->_screen_id = web_app->_screen_id;

              g_strfreev (splitted_ip);

              g_idle_add ((GSourceFunc) OnNewClient,
                          client);
            }
          }
        }

        server->_clients = g_list_prepend (server->_clients,
                                           web_app);
      }
      break;

      case LWS_CALLBACK_CLOSED:
      {
        printf (RED "LWS_CALLBACK_CLOSED      %p/%p\n" ESC, (void *) web_app, (void *) wsi);

        delete (web_app->_input_buffer);
        delete (web_app->_pending_message);

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
          g_idle_add ((GSourceFunc) OnRequest,
                      web_app->_input_buffer);
          web_app->_input_buffer = nullptr;
        }
      }
      break;

      case LWS_CALLBACK_SERVER_WRITEABLE:
      {
        if (web_app->_pending_message)
        {
          GString *buffer = g_string_new (nullptr);

          g_string_append_len (buffer,
                               "", LWS_PRE);

          g_string_append_printf (buffer,
                                  "%s", web_app->_pending_message->_message);

          {
            guchar *msg = (guchar *) &buffer->str[LWS_PRE];

            lws_write (wsi,
                       msg,
                       strlen ((gchar *) msg),
                       LWS_WRITE_TEXT);
          }

          g_string_free (buffer,
                         TRUE);

          delete (web_app->_pending_message);
          web_app->_pending_message = nullptr;
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
    server->_context = lws_create_context (server->_info);

    if (server->_context)
    {
      while (server->_running)
      {
        gboolean ready = TRUE;

        if (g_async_queue_length (server->_queue))
        {
          lws_service (server->_context, 100);
        }
        else
        {
          lws_service (server->_context, G_MAXINT);
        }

        for (GList *current = server->_clients; current != nullptr; current = g_list_next (current))
        {
          WebApp *web_app = (WebApp *) current->data;

          if (web_app->_pending_message != nullptr)
          {
            ready = FALSE;
            break;
          }
        }

        if (ready)
        {
          OutgoingMessage *outgoing_message = (OutgoingMessage *) g_async_queue_timeout_pop (server->_queue,
                                                                                             1000);
          if (outgoing_message)
          {
            for (GList *current = server->_clients; current != nullptr; current = g_list_next (current))
            {
              WebApp *web_app = (WebApp *) current->data;

              if (   (outgoing_message->_client_id == 0)
                  || (web_app->_id == outgoing_message->_client_id))
              {
                web_app->_pending_message = outgoing_message->Duplicate ();

                lws_callback_on_writable (web_app->_wsi);

                if (outgoing_message->_client_id != 0)
                {
                  break;
                }
              }
            }

            delete outgoing_message;
          }
        }
      }

      lws_context_destroy (server->_context);
    }

    return nullptr;
  }
}
