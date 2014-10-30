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

#include <stdlib.h>
#include <glib/gstdio.h>
#include <gio/gunixsocketaddress.h>

#include "wpa.hpp"

// --------------------------------------------------------------------------------
Wpa::Wpa ()
  : Object ("Wpa")
{
  _network_config = NULL;
  _cancellable    = NULL;

  OpenSocket ();
}

// --------------------------------------------------------------------------------
Wpa::~Wpa ()
{
  if (_cancellable)
  {
    g_cancellable_cancel (_cancellable);
    g_object_unref (_cancellable);
  }

  if (_socket)
  {
    {
      GError *error = NULL;

      if (g_socket_close (_socket,
                          &error) == -1)
      {
        g_warning ("%s", error->message);
        g_error_free (error);
      }
      g_object_unref (_socket);
    }

    {
      gchar *path = g_strdup_printf ("/tmp/wpa_ctrl_%d-1", getpid ());

      g_remove (path);
      g_free (path);
    }
  }

  g_free (_network_config);
}

// --------------------------------------------------------------------------------
void Wpa::OpenSocket ()
{
  GError *error = NULL;

  _socket = g_socket_new (G_SOCKET_FAMILY_UNIX,
                          G_SOCKET_TYPE_DATAGRAM,
                          G_SOCKET_PROTOCOL_DEFAULT,
                          &error);
  if (_socket == NULL)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
  }

  if (_socket)
  {
    // request
    {
      gchar          *path            = g_strdup_printf ("/tmp/wpa_ctrl_%d-1", getpid ());
      GSocketAddress *request_address = g_unix_socket_address_new (path);

      g_free (path);

      if (g_socket_bind (_socket,
                         request_address,
                         FALSE,
                         &error) == FALSE)
      {
        g_warning ("%s", error->message);
        g_error_free (error);
      }

      g_object_unref (request_address);
    }

    // answer
    {
      GSocketAddress *answer_address = g_unix_socket_address_new ("/var/run/wpa_supplicant/wlan0");

      if (g_socket_connect (_socket,
                            answer_address,
                            FALSE,
                            &error) == FALSE)
      {
        g_warning ("%s: %s", "/var/run/wpa_supplicant/wlan0", error->message);
        g_error_free (error);
      }
    }
  }
}

// --------------------------------------------------------------------------------
gchar *Wpa::Send (const gchar *request)
{
  GError *error = NULL;

  printf (YELLOW "%s " ESC, request);

  if (g_socket_send (_socket,
                     request,
                     strlen (request) + 1,
                     _cancellable,
                     &error) == -1)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
  }
  else
  {
    gsize  size   = 100;
    gchar *answer = g_new0 (gchar, size);

    g_socket_receive (_socket,
                      answer,
                      size-1,
                      _cancellable,
                      &error);
    if (error)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
      g_free (answer);
    }
    else
    {
      printf (RED "%s" ESC, answer);
      return answer;
    }
  }

  printf ("\n");
  return NULL;
}

// --------------------------------------------------------------------------------
gchar *Wpa::GetNetworkConfig ()
{
#ifdef WIRING_PI
  GError *error  = NULL;
  gchar  *config = NULL;

  if (g_file_get_contents ("/media/usb1/bellepoule.wifi",
                           &config,
                           NULL,
                           &error) == FALSE)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
  }
  else if (   (_network_config == NULL)
           || (strcmp (_network_config, config) != 0))
  {
    return config;
  }

  g_free (config);
#else
    sleep (10);
#endif

  return NULL;
}

// --------------------------------------------------------------------------------
gchar *Wpa::AddNetwork ()
{
  gchar *network_id = Send ("ADD_NETWORK");

  if (network_id)
  {
    gchar *new_line   = strchr (network_id, '\n');

    if (new_line)
    {
      *new_line = '\0';
    }
  }

  return network_id;
}

// --------------------------------------------------------------------------------
void Wpa::ConfigureNetwork (GSourceFunc  configuration_cbk,
                            Object      *configuration_client)
{
  _configuration_cbk    = configuration_cbk;
  _configuration_client = configuration_client;

  g_thread_new (NULL,
                (GThreadFunc) ConfigurationThread,
                this);
}

// --------------------------------------------------------------------------------
gpointer Wpa::ConfigurationThread (Wpa *wpa)
{
  gchar *new_config = wpa->GetNetworkConfig ();

  if (new_config)
  {
    GError   *error    = NULL;
    GKeyFile *key_file = g_key_file_new ();

    if (g_key_file_load_from_file (key_file,
                                   "/media/usb1/bellepoule.wifi",
                                   G_KEY_FILE_NONE,
                                   &error) == FALSE)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }
    else
    {
      gchar **keys;

      keys = g_key_file_get_keys (key_file,
                                  "wpa_supplicant",
                                  NULL,
                                  &error);
      if (error)
      {
        g_warning ("%s", error->message);
        g_error_free (error);
      }
      else if (keys)
      {
        gchar *network_id;

        {
          gchar *last_network = wpa->AddNetwork ();

          for (gint i = 1 ; i <= atoi (last_network); i++)
          {
            gchar *request = g_strdup_printf ("REMOVE_NETWORK %d", i);

            wpa->Send (request);
            g_free (request);
          }
        }

        network_id = wpa->AddNetwork ();

        for (guint k = 0; keys[k]; k++)
        {
          gchar *value;

          value = g_key_file_get_string (key_file,
                                         "wpa_supplicant",
                                         keys[k],
                                         &error);
          if (error)
          {
            g_warning ("%s", error->message);
            g_error_free (error);
            error = NULL;
          }
          else
          {
            gchar *request = g_strdup_printf ("SET_NETWORK %s %s %s", network_id, keys[k], value);

            wpa->Send (request);

            g_free (value);
            g_free (request);
          }
        }

        {
          gchar *request;

          request = g_strdup_printf ("ENABLE_NETWORK %s", network_id);
          g_free (wpa->Send (request));
          g_free (request);

          request = g_strdup_printf ("SELECT_NETWORK %s", network_id);
          g_free (wpa->Send (request));
          g_free (request);
        }

        g_free (wpa->Send ("SAVE_CONFIG"));

        g_free (network_id);

        g_free (wpa->_network_config);
        wpa->_network_config = new_config;
      }
    }
    g_key_file_unref (key_file);
  }

  g_idle_add (wpa->_configuration_cbk,
              wpa->_configuration_client);

  return NULL;
}
