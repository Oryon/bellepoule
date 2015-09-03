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

#include "ring.hpp"
#include "message.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  Message::Message (const gchar *name)
    : Object ("Message")
  {
    _key_file = g_key_file_new ();

    g_key_file_set_string (_key_file,
                           "Header",
                           "name",
                           name);

    _is_valid = TRUE;
  }

  // --------------------------------------------------------------------------------
  Message::Message (const guint8       *data,
                    struct sockaddr_in *from)
  {
    GError *error = NULL;

    _is_valid = TRUE;
    _key_file = g_key_file_new ();

    if (g_key_file_load_from_data (_key_file,
                                   (const gchar *) data,
                                   -1,
                                   G_KEY_FILE_NONE,
                                   &error) == FALSE)
    {
      g_warning ("Message::Message: %s", error->message);
      _is_valid = FALSE;
      g_clear_error (&error);
    }

    if (from)
    {
      g_key_file_set_string (_key_file,
                             "Header",
                             "from",
                             inet_ntoa (from->sin_addr));
    }
  }

  // --------------------------------------------------------------------------------
  Message::~Message ()
  {
    g_key_file_free (_key_file);
  }

  // --------------------------------------------------------------------------------
  gboolean Message::IsValid ()
  {
    return _is_valid;
  }

  // --------------------------------------------------------------------------------
  gboolean Message::Is (const gchar *name)
  {
    gboolean  result;
    gchar    *field_name = g_key_file_get_string (_key_file,
                                                  "Header",
                                                  "name",
                                                  NULL);

    if (name && field_name)
    {
      result = (strcmp (name, field_name) == 0);
    }
    else
    {
      result = (name == field_name);
    }

    g_free (field_name);
    return result;
  }

  // --------------------------------------------------------------------------------
  gchar *Message::GetSenderIp ()
  {
    return g_key_file_get_string (_key_file,
                                  "Header",
                                  "from",
                                  NULL);
  }

  // --------------------------------------------------------------------------------
  void Message::Spread ()
  {
    Ring::SpreadMessage (this);
  }

  // --------------------------------------------------------------------------------
  void Message::Recall ()
  {
    Ring::RecallMessage (this);
  }

  // --------------------------------------------------------------------------------
  void Message::Set (const gchar *field,
                     const gchar *value)
  {
    g_key_file_set_string (_key_file,
                           "Body",
                           field,
                           value);
    _is_valid = TRUE;
  }

  // --------------------------------------------------------------------------------
  void Message::Set (const gchar *field,
                     const guint  value)
  {
    g_key_file_set_integer (_key_file,
                            "Body",
                            field,
                            value);
    _is_valid = TRUE;
  }

  // --------------------------------------------------------------------------------
  gchar *Message::GetString (const gchar *field)
  {
    return g_key_file_get_string (_key_file,
                                  "Body",
                                  field,
                                  NULL);
  }

  // --------------------------------------------------------------------------------
  guint Message::GetInteger (const gchar *field)
  {
    return g_key_file_get_integer (_key_file,
                                   "Body",
                                   field,
                                   NULL);
  }

  // --------------------------------------------------------------------------------
  gchar *Message::GetParcel ()
  {
    return g_key_file_to_data (_key_file,
                               NULL,
                               NULL);
  }

  // --------------------------------------------------------------------------------
  void Message::Dump ()
  {
    gchar *parcel = GetParcel ();

    printf (BLUE "-----------------\n" ESC);
    printf ("%s\n", parcel);
    g_free (parcel);
  }
}
