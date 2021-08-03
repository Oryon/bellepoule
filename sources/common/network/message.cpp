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
  Message::Message ()
    : Object ("Message")
  {
    _spread        = FALSE;
    _passphrase256 = nullptr;
    _is_valid      = TRUE;
    _key_file      = g_key_file_new ();
  }

  // --------------------------------------------------------------------------------
  Message::Message (const gchar *name)
    : Message ()
  {
    g_key_file_set_string (_key_file,
                           "Header",
                           "name",
                           name);

    g_key_file_set_uint64 (_key_file,
                           "Header",
                           "netid",
                           g_random_int ());
  }

  // --------------------------------------------------------------------------------
  Message::Message (const guint8 *data)
    : Message ()
  {
    if (data)
    {
      GError *error = nullptr;

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
    }
  }

  // --------------------------------------------------------------------------------
  Message::~Message ()
  {
    g_free (_passphrase256);
    g_key_file_free (_key_file);
  }

  // --------------------------------------------------------------------------------
  Message *Message::Clone ()
  {
    Message *clone = new Message ();

    clone->_is_valid = _is_valid;
    clone->SetPassPhrase256 (_passphrase256);

    {
      gchar *data = g_key_file_to_data (_key_file,
                                        nullptr,
                                        nullptr);

      g_key_file_load_from_data (clone->_key_file,
                                 data,
                                 -1,
                                 G_KEY_FILE_NONE,
                                 nullptr);
    }

    return clone;
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
                                                  nullptr);

    result = (g_strcmp0 (name, field_name) == 0);

    g_free (field_name);
    return result;
  }

  // --------------------------------------------------------------------------------
  void Message::SetNetID (guint id)
  {
    g_key_file_set_uint64 (_key_file,
                           "Header",
                           "netid",
                           id);
  }

  // --------------------------------------------------------------------------------
  guint Message::GetNetID ()
  {
    return g_key_file_get_uint64 (_key_file,
                                  "Header",
                                  "netid",
                                  nullptr);
  }

  // --------------------------------------------------------------------------------
  gboolean Message::IsSpread ()
  {
    return _spread;
  }

  // --------------------------------------------------------------------------------
  void Message::Recall ()
  {
    Ring::_broker->RecallMessage (this);
    _spread = FALSE;
  }

  // --------------------------------------------------------------------------------
  void Message::SetPassPhrase256 (const gchar *passphrase256)
  {
    g_free (_passphrase256);
    _passphrase256 = nullptr;

    if (passphrase256)
    {
      _passphrase256 = g_new (gchar, 256/8 + 1);

      memcpy (_passphrase256,
              passphrase256,
              (256/8) + 1);
    }
  }

  // --------------------------------------------------------------------------------
  const gchar *Message::GetPassPhrase256 ()
  {
    return _passphrase256;
  }

  // --------------------------------------------------------------------------------
  void Message::SetFitness (const guint value)
  {
    g_key_file_set_uint64 (_key_file,
                           "Header",
                           "fitness",
                           value);
    _is_valid = TRUE;
  }

  // --------------------------------------------------------------------------------
  guint Message::GetFitness ()
  {
    return g_key_file_get_uint64 (_key_file,
                                  "Header",
                                  "fitness",
                                  nullptr);
  }

  // --------------------------------------------------------------------------------
  void Message::Spread ()
  {
    Ring::_broker->SpreadMessage (this);
    _spread = TRUE;
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
    g_key_file_set_uint64 (_key_file,
                           "Body",
                           field,
                           value);
    _is_valid = TRUE;
  }

  // --------------------------------------------------------------------------------
  void Message::SetList (const gchar *field,
                         const guint  list[],
                         gsize        length)
  {
    g_key_file_set_integer_list (_key_file,
                                 "Body",
                                 field,
                                 (gint *) list,
                                 length);
    _is_valid = TRUE;
  }

  // --------------------------------------------------------------------------------
  void Message::Set (const gchar *field,
                     const gint   value)
  {
    g_key_file_set_int64 (_key_file,
                          "Body",
                          field,
                          value);
    _is_valid = TRUE;
  }

  // --------------------------------------------------------------------------------
  void Message::Remove (const gchar *field)
  {
    g_key_file_remove_key (_key_file,
                           "Body",
                           field,
                           nullptr);
  }

  // --------------------------------------------------------------------------------
  gboolean Message::HasField (const gchar *field)
  {
    return g_key_file_has_key (_key_file,
                               "Body",
                               field,
                               nullptr);
  }

  // --------------------------------------------------------------------------------
  gchar *Message::GetString (const gchar *field)
  {
    return g_key_file_get_string (_key_file,
                                  "Body",
                                  field,
                                  nullptr);
  }

  // --------------------------------------------------------------------------------
  guint Message::GetInteger (const gchar *field)
  {
    return g_key_file_get_uint64 (_key_file,
                                  "Body",
                                  field,
                                  nullptr);
  }

  // --------------------------------------------------------------------------------
  guint *Message::GetIntegerList (const gchar *field,
                                  gsize       *length)
  {
    return (guint *) g_key_file_get_integer_list (_key_file,
                                                  "Body",
                                                  field,
                                                  length,
                                                  nullptr);
  }

  // --------------------------------------------------------------------------------
  gint Message::GetSignedInteger (const gchar *field)
  {
    return g_key_file_get_uint64 (_key_file,
                                  "Body",
                                  field,
                                  nullptr);
  }

  // --------------------------------------------------------------------------------
  gchar *Message::GetParcel ()
  {
    return g_key_file_to_data (_key_file,
                               nullptr,
                               nullptr);
  }

  // --------------------------------------------------------------------------------
  void Message::Log (gboolean regular)
  {
    if (regular)
    {
      gchar *parcel = GetParcel ();

      printf (BLUE "-----------------\n" ESC);
      printf ("%s\n", parcel);
      g_free (parcel);
    }
    else
    {
      gchar *field_name = g_key_file_get_string (_key_file,
                                                 "Header",
                                                 "name",
                                                 nullptr);

      if (GetFitness ())
      {
        printf (BLUE "< %s >\n" ESC, field_name);
      }
      else
      {
        printf (RED "< %s >\n" ESC, field_name);
      }
      g_free (field_name);
    }
  }
}
