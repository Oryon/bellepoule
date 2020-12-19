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
#include "usb_drive.hpp"

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#include "usb_broker.hpp"

namespace Net
{
  // ----------------------------------------------------------------------------------------------
  UsbBroker::UsbBroker (Listener *listener)
    : Object ("UsbBroker")
  {
    _listener       = listener;
    _stop_channel   = nullptr;
    _in_channel     = nullptr;
    _sources        = nullptr;
    _json           = g_string_new ("");

    g_datalist_init (&_drives);

    Start ();
  }

  // ----------------------------------------------------------------------------------------------
  UsbBroker::~UsbBroker ()
  {
    g_datalist_clear (&_drives);

    for (GList *current = _sources; current; current = g_list_next (current))
    {
      g_source_remove (GPOINTER_TO_UINT (current->data));
    }
    g_list_free (_sources);

    if (_stop_channel)
    {
      g_io_channel_unref (_stop_channel);
    }

    if (_in_channel)
    {
      g_io_channel_unref (_in_channel);
    }

#ifdef G_OS_WIN32
    TerminateProcess (_child_pid,
                      0);
#endif
    g_spawn_close_pid (_child_pid);

    g_string_free (_json,
                   TRUE);

    Reset ();
  }

  // ----------------------------------------------------------------------------------------------
  void UsbBroker::Start ()
  {
    gint    child_stdin;
    gint    child_stderr;
    GError *error        = nullptr;

    {
#if defined(PRODUCT)
#ifdef G_OS_WIN32
      gchar *path = g_build_filename (Global::_binary_dir, PRODUCT "-backend.exe", nullptr);
#else
      gchar *path = g_build_filename (Global::_binary_dir, PRODUCT "-backend", nullptr);
#endif
#else
      gchar *path = g_build_filename (Global::_binary_dir, "backend", nullptr);
#endif
      gchar **argv = g_new0 (gchar *, 2);

      argv[0] = path;

      if (g_spawn_async_with_pipes (nullptr,
                                    argv,
                                    nullptr,
#ifdef G_OS_WIN32
                                    G_SPAWN_DO_NOT_REAP_CHILD,
#else
                                    G_SPAWN_DEFAULT,
#endif
                                    nullptr,
                                    nullptr,
                                    &_child_pid,
                                    &child_stdin,
                                    nullptr,
                                    &child_stderr,
                                    &error) == FALSE)
      {
        g_warning ("g_spawn_async_with_pipes: %s\n", error->message);
        g_clear_error (&error);
      }

      g_free (argv);
      g_free (path);
    }

    _stop_channel = GetIoChannel (child_stdin,
                                  (GIOCondition) 0,
                                  nullptr);
    _in_channel = GetIoChannel (child_stderr,
                                (GIOCondition) (G_IO_IN|G_IO_PRI),
                                (GIOFunc) OnUsbBackendData);
  }

  // ----------------------------------------------------------------------------------------------
  void UsbBroker::Reset ()
  {
  }

  // ----------------------------------------------------------------------------------------------
  GIOChannel *UsbBroker::GetIoChannel (gint         fd,
                                       GIOCondition cond,
                                       GIOFunc      func)
  {
    GIOChannel *channel;

#ifdef G_OS_WIN32
    channel = g_io_channel_win32_new_fd (fd);
#else
    channel = g_io_channel_unix_new (fd);
#endif

    g_io_channel_set_close_on_unref (channel,
                                     TRUE);

    if (func)
    {
      guint source_id = g_io_add_watch (channel,
                                        cond,
                                        func,
                                        this);
      _sources = g_list_prepend (_sources,
                                 GUINT_TO_POINTER (source_id));
    }

    return channel;
  }

  // ----------------------------------------------------------------------------------------------
  gchar *UsbBroker::GetField (JsonParser  *parser,
                              const gchar *field_name)
  {
    const gchar *field  = nullptr;
    JsonReader  *reader = json_reader_new (json_parser_get_root (parser));

    if (json_reader_read_member (reader, field_name))
    {
      field = json_reader_get_string_value (reader);
    }
    json_reader_end_member (reader);

    g_object_unref (reader);

    return g_strdup (field);
  }

  // ----------------------------------------------------------------------------------------------
  void UsbBroker::OnEvent ()
  {
    GError     *error  = nullptr;
    JsonParser *parser = json_parser_new ();

    if (json_parser_load_from_data (parser,
                                    _json->str,
                                    -1,
                                    &error))
    {
      gchar *event = GetField (parser, "Event");

      Retain ();
      if (g_strcmp0 (event, "ERROR") == 0)
      {
        g_warning ("%s", GetField (parser, "Message"));
      }
      else if (g_strcmp0 (event, "PLUG") == 0)
      {
        UsbDrive *drive = new UsbDrive (GetField (parser, "Slot"),
                                        GetField (parser, "Product"),
                                        GetField (parser, "Manufacturer"),
                                        GetField (parser, "SerialNumber"));

        g_datalist_set_data_full (&_drives,
                                  drive->GetSlot (),
                                  drive,
                                  (GDestroyNotify) Object::TryToRelease);

        _listener->OnUsbEvent (Event::STICK_PLUGGED,
                               drive);
      }
      else if (g_strcmp0 (event, "UNPLUG") == 0)
      {
        gchar    *slot  = GetField (parser, "Slot");
        UsbDrive *drive = (UsbDrive *) g_datalist_get_data (&_drives,
                                                            slot);

        if (drive)
        {
          _listener->OnUsbEvent (Event::STICK_UNPLUGGED,
                                 drive);
          g_datalist_remove_data (&_drives,
                                  drive->GetSlot ());
        }
        g_free (slot);
      }
      g_free (event);
      Release ();
    }

    if (error)
    {
      g_warning ("json_parser_load_from_data: %s\n", error->message);
      g_clear_error (&error);
    }

    g_object_unref (parser);
  }

  // ----------------------------------------------------------------------------------------------
  gboolean UsbBroker::OnUsbBackendData (GIOChannel   *channel,
                                        GIOCondition  cond,
                                        UsbBroker    *broker)
  {
    if (cond & (G_IO_IN | G_IO_PRI))
    {
      GError *error = nullptr;
      gchar  *line;

      if (g_io_channel_read_line (channel,
                                  &line,
                                  nullptr,
                                  nullptr,
                                  &error) == G_IO_STATUS_NORMAL)
      {
        if (line)
        {
          if (line[0] == '.')
          {
            broker->OnEvent ();
            g_string_free (broker->_json,
                           TRUE);
            broker->_json = g_string_new ("");
          }
          else
          {
            g_string_append (broker->_json,
                             line);
          }

          g_free (line);
        }
        return TRUE;
      }

      if (error)
      {
        g_warning ("g_io_channel_read_line: %s\n", error->message);
        g_clear_error (&error);
      }
    }

    return FALSE;
  }
}
