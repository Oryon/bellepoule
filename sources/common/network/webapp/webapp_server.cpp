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
#include "pattern.hpp"
#include "message.hpp"
#include "webapp_server.hpp"

namespace Net
{
  WebAppServer::Resource WebAppServer::_resources[] = 
  {
    {".js",    "js",    "application/javascript"},
    {".css",   "css",   "text/css"},
    {".ttf",   "fonts", "application/x-font-ttf"},
    {".png",   "",      "image/png"},
    {".html",  "",      "text/html"},
    {nullptr,  nullptr, nullptr}
  };

  // --------------------------------------------------------------------------------
  // --------------------------------------------------------------------------------
  WebAppServer::IncomingRequest::IncomingRequest (Server *server,
                                                  guint   id)
    : RequestBody (server)
  {
    _client_uuid = id;
  }

  // --------------------------------------------------------------------------------
  WebAppServer::IncomingRequest::~IncomingRequest ()
  {
  }

  // --------------------------------------------------------------------------------
  // --------------------------------------------------------------------------------
  WebAppServer::OutgoingMessage::OutgoingMessage (guint    client_uuid,
                                                  Message *message)
  {
    _client_uuid = client_uuid;
    _message     = message->GetParcel ();
  }

  // --------------------------------------------------------------------------------
  WebAppServer::OutgoingMessage::OutgoingMessage (guint        client_uuid,
                                                  const gchar *message)
  {
    _client_uuid = client_uuid;
    _message     = g_strdup (message);
  }

  // --------------------------------------------------------------------------------
  WebAppServer::OutgoingMessage::~OutgoingMessage ()
  {
    g_free (_message);
  }

  // --------------------------------------------------------------------------------
  WebAppServer::OutgoingMessage *WebAppServer::OutgoingMessage::Duplicate ()
  {
    return new OutgoingMessage (_client_uuid,
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
    _aliases    = nullptr;
    _channel    = channel;
    _ip_address = nullptr;

    _piste_pattern = new Pattern ("\\/arene\\/([0-9]+)$",
                                  "<div>L'arène <b><font color=\"blue\">%d</font></b> est déjà réservée.</div>");

    _referee_pattern = new Pattern ("\\/arbitre\\/arene\\/([0-9]+)$",
                                    "<div>L'arène <b><font color=\"blue\">%d</font></b> n'existe pas.</div>");

    _standalone_referee_pattern = new Pattern ("\\/arbitre$",
                                               nullptr);

    _audience_pattern = new Pattern ("\\/public$",
                                     nullptr);

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

    _piste_pattern->Release ();
    _referee_pattern->Release ();
    _standalone_referee_pattern->Release ();
    _audience_pattern->Release ();

    g_async_queue_unref (_queue);

    g_list_free (_clients);
    g_list_free (_aliases);
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
  gboolean WebAppServer::OnClientClosed (IncomingRequest *request)
  {
    WebAppServer *server  = dynamic_cast<WebAppServer *> (request->_server);

    for (GList *m = server->_aliases; m; m = g_list_next (m))
    {
      IdMap *map = (IdMap *) m->data;

      if (map->_uuid == request->_client_uuid)
      {
        server->_aliases = g_list_remove_link (server->_aliases,
                                               m);
        break;
      }
    }
    return G_SOURCE_REMOVE;
  }

  // --------------------------------------------------------------------------------
  gboolean WebAppServer::OnIncomingMessage (IncomingRequest *request)
  {
    WebAppServer *server  = dynamic_cast<WebAppServer *> (request->_server);
    Message      *message = new Message ((const guint8 *) request->_data);

    if (message->Is ("SmartPoule::RegisterPiste"))
    {
      server->OnNewMirror (request->_client_uuid,
                           message->GetInteger ("number"));
    }
    else if (message->HasField ("mirror"))
    {
      guint mirror = message->GetInteger ("mirror");

      for (GList *m = server->_aliases; m; m = g_list_next (m))
      {
        IdMap *map = (IdMap *) m->data;

        if (map->_alias == mirror)
        {
          server->SendMessageTo (map->_uuid,
                                 message);
          break;
        }
      }
    }
    else
    {
      message->Set ("client",  request->_client_uuid);
      message->Set ("channel", server->_channel);
      server->_listener->OnMessage (message);
    }

    message->Release ();
    delete (request);
    return G_SOURCE_REMOVE;
  }

  // --------------------------------------------------------------------------------
  void WebAppServer::OnNewMirror (guint uuid,
                                  guint alias)
  {
    {
      IdMap *map = g_new (IdMap, 1);

      map->_uuid  = uuid;
      map->_alias = alias;
      _aliases = g_list_prepend (_aliases,
                                 map);
    }

    {
      GdkPixbuf *pixbuf;
      gchar     *png;
      gsize      png_size;
      gchar     *png64;
      GError    *error    = nullptr;
      Message   *message  = new Message ("BellePoule::QrCode");

      {
        gchar *code = g_strdup_printf ("http://%s:%d/arbitre/arene/%d",
                                       _ip_address,
                                       GetPort (),
                                       alias);

        pixbuf = FlashCode::GetPixbuf (code);
        message->Set ("qrcode.txt", code);

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
      message->Set ("qrcode.png", png64);

      g_free (png);
      g_object_unref (pixbuf);
      g_free (png64);

      SendMessageTo (uuid,
                     message);
    }
  }

  // --------------------------------------------------------------------------------
  int WebAppServer::OnHttp (struct lws                *wsi,
                            enum lws_callback_reasons  reason,
                            void                      *user,
                            gchar                     *in,
                            size_t                     len)
  {
    WebAppServer *server = nullptr;

    if ((WebAppServer *) lws_get_protocol (wsi))
    {
      server = ((WebAppServer *) (lws_get_protocol (wsi)->user));

#pragma GCC diagnostic ignored "-Wswitch-enum"
      switch (reason)
      {
        case LWS_CALLBACK_HTTP:
        {
          gint n;

          for (Resource *r = _resources; r->suffix; r++)
          {
            if (g_str_has_suffix (in, r->suffix))
            {
              gchar *base_name = g_path_get_basename (in);
              gchar *app_path  = g_build_filename (Global::_share_dir, "resources", "webapps", r->folder, base_name, nullptr);

              n = lws_serve_http_file (wsi,
                                       app_path,
                                       r->mime_type,
                                       nullptr, 0);
              g_free (app_path);
              g_free (base_name);
              return 0;
            }
          }

          if (   (server->_standalone_referee_pattern->Match (in) == FALSE)
              && (server->_audience_pattern->Match (in) == FALSE))
          {
            if (server->_piste_pattern->Match (in))
            {
              guint alias = server->_piste_pattern->GetArgument ();

              for (GList *m = server->_aliases; m; m = g_list_next (m))
              {
                IdMap *map = (IdMap *) m->data;

                if (map->_alias == alias)
                {
                  gchar *error = server->_piste_pattern->GetErrorString ();

                  lws_return_http_status (wsi,
                                          HTTP_STATUS_BAD_REQUEST,
                                          error);
                  g_free (error);
                  return 0;
                }
              }
            }
            else if (server->_referee_pattern->Match (in))
            {
              guint alias = server->_referee_pattern->GetArgument ();
              IdMap *map  = nullptr;

              for (GList *m = server->_aliases; m; m = g_list_next (m))
              {
                IdMap *current_map = (IdMap *) m->data;

                if (current_map->_alias == alias)
                {
                  map = current_map;
                  break;
                }
              }

              if (map == nullptr)
              {
                gchar *error = server->_referee_pattern->GetErrorString ();

                lws_return_http_status (wsi,
                                        HTTP_STATUS_BAD_REQUEST,
                                        error);
                g_free (error);
                return 0;
              }
            }
            else
            {
              GString *body = g_string_new (nullptr);

              g_string_append        (body, "<h2 style=\"color: #2e6c80;\">Mauvaise requ&egrave;te :</h2>");
              g_string_append        (body, "<p>L'adresse que vous venez de saisir n'a pas pas &eacute;t&eacute; comprise par le logiciel BellePoule.&nbsp;</p>");
              g_string_append        (body, "<p>Corrigez l&agrave; en vous conformant &agrave; l'un des 4 formats possibles. &nbsp;</p>");
              g_string_append        (body, "<h2 style=\"color: #2e6c80;\">Les 4 formats d'adresses possibles (*):</h2>");
              g_string_append        (body, "<table bgcolor=\"#FFF8C9\">");
              g_string_append        (body, "<tbody>");
              g_string_append        (body, "<tr>");
              g_string_append_printf (body, "<td>http://%s:8000/arene/<strong><span style=\"color: #0000ff;\">XX</span></strong></td>", server->_ip_address);
              g_string_append        (body, "<td>&#10145; Retour sur ar&egrave;ne</td>");
              g_string_append        (body, "</tr>");
              g_string_append        (body, "<tr>");
              g_string_append_printf (body, "<td>http://%s:8000/arbitre/arene/<strong><span style=\"color: #0000ff;\">XX</span></strong></td>", server->_ip_address);
              g_string_append        (body, "<td>&#10145; Application d'arbitrage <strong>avec</strong> retour sur ar&egrave;ne</td>");
              g_string_append        (body, "</tr>");
              g_string_append        (body, "<tr>");
              g_string_append_printf (body, "<td>http://%s:8000/arbitre</td>", server->_ip_address);
              g_string_append        (body, "<td>&#10145; Application d'arbitrage <strong>sans</strong> retour sur ar&egrave;ne</td>");
              g_string_append        (body, "</tr>");
              g_string_append        (body, "<tr>");
              g_string_append_printf (body, "<td>http://%s:8000/public</td>", server->_ip_address);
              g_string_append        (body, "<td>&#10145; Affichage pour le public</td>");
              g_string_append        (body, "</tr>");
              g_string_append        (body, "</tbody>");
              g_string_append        (body, "</table>");
              g_string_append        (body, "<p><em>* Remplacer <span style=\"color: #0000ff;\"><strong>XX</strong></span> par le num&eacute;ro d'arène souhait&eacute;.</em></p>");

              lws_return_http_status (wsi,
                                      HTTP_STATUS_BAD_REQUEST,
                                      body->str);
              g_string_free (body,
                             TRUE);
              return 0;
            }
          }

          {
            gchar *app_path = g_build_filename (Global::_share_dir, "resources", "webapps", "smartpoule.html", nullptr);

            n = lws_serve_http_file (wsi,
                                     app_path,
                                     "text/html",
                                     nullptr, 0);
            g_free (app_path);
          }

          if (n < 0 || ((n > 0) && lws_http_transaction_completed (wsi)))
          {
            return -1;
          }
        }
        break;

        case LWS_CALLBACK_HTTP_FILE_COMPLETION:
        {
          if (lws_http_transaction_completed (wsi))
          {
            return -1;
          }
        }
        break;

        default:
        break;
      }
#pragma GCC diagnostic pop
    }

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
              web_app->_uuid = g_ascii_strtoull (splitted_ip[3],
                                                 nullptr,
                                                 10);
              g_strfreev (splitted_ip);
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

        {
          IncomingRequest *request = new IncomingRequest (server,
                                                          web_app->_uuid);

          g_idle_add ((GSourceFunc) OnClientClosed,
                      request);
        }
      }
      break;

      case LWS_CALLBACK_RECEIVE:
      {
        if (web_app->_input_buffer == nullptr)
        {
          web_app->_input_buffer = new IncomingRequest (server,
                                                        web_app->_uuid);
        }

        web_app->_input_buffer->Append (in,
                                        len);

        if (lws_is_final_fragment (wsi))
        {
          web_app->_input_buffer->ZeroTerminate ();
          g_idle_add ((GSourceFunc) OnIncomingMessage,
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

              if (   (outgoing_message->_client_uuid == 0)
                  || (web_app->_uuid == outgoing_message->_client_uuid))
              {
                web_app->_pending_message = outgoing_message->Duplicate ();

                lws_callback_on_writable (web_app->_wsi);

                if (outgoing_message->_client_uuid != 0)
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
