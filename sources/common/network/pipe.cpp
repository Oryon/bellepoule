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

#include "pipe.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  Pipe::Pipe (GSocketConnection *connection)
  {
    _connection = connection;
    g_object_ref (_connection);

    _buffer = g_new (char, BUFFER_SIZE);

    {
      GInputStream *stream = g_io_stream_get_input_stream (G_IO_STREAM (connection));

      g_input_stream_read_async (stream,
                                 _buffer,
                                 BUFFER_SIZE,
                                 G_PRIORITY_DEFAULT,
                                 NULL,
                                 (GAsyncReadyCallback) OnDataInReady,
                                 this);
    }
  }

  // --------------------------------------------------------------------------------
  Pipe::~Pipe ()
  {
    g_object_unref (_connection);
    g_free         (_buffer);
  }

  // -------------------------------------------------------------------------------
  void Pipe::OnDataInReady (GInputStream *in,
                            GAsyncResult *result,
                            Pipe         *pipe)
  {
    GError *error = NULL;

    g_input_stream_read_finish (in,
                                result,
                                &error);
    if (error != NULL)
    {
      g_warning ("%s", error->message);
      g_clear_error (&error);
    }

    printf ("%s\n", pipe->_buffer);

    g_input_stream_read_async (in,
                               pipe->_buffer,
                               BUFFER_SIZE,
                               G_PRIORITY_DEFAULT,
                               NULL,
                               (GAsyncReadyCallback) OnDataInReady,
                               pipe);
  }

}
