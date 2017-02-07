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
    _passphrase         = NULL;
    _is_valid           = TRUE;
    _waiting_to_be_sent = FALSE;
    _key_file           = g_key_file_new ();
  }

  // --------------------------------------------------------------------------------
  Message::Message (const gchar *name)
    : Message ()
  {
    g_key_file_set_string (_key_file,
                           "Header",
                           "name",
                           name);

    g_key_file_set_string (_key_file,
                           "Header",
                           "sender",
                           Ring::GetRole ());

    g_key_file_set_integer (_key_file,
                            "Header",
                            "netid",
                            g_random_int ());
  }

  // --------------------------------------------------------------------------------
  Message::Message (const guint8 *data)
    : Message ()
  {
    GError *error = NULL;

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

  // --------------------------------------------------------------------------------
  Message::~Message ()
  {
    g_free (_passphrase);
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

    result = (g_strcmp0 (name, field_name) == 0);

    g_free (field_name);
    return result;
  }

  // --------------------------------------------------------------------------------
  void Message::SetNetID (guint id)
  {
    g_key_file_set_integer (_key_file,
                            "Header",
                            "netid",
                            id);
  }

  // --------------------------------------------------------------------------------
  guint Message::GetNetID ()
  {
    return g_key_file_get_integer (_key_file,
                                   "Header",
                                   "netid",
                                   NULL);
  }

  // --------------------------------------------------------------------------------
  gchar *Message::GetSender ()
  {
    return g_key_file_get_string (_key_file,
                                  "Header",
                                  "sender",
                                  NULL);
  }

  // --------------------------------------------------------------------------------
  void Message::Recall ()
  {
    Ring::RecallMessage (this);
  }

  // --------------------------------------------------------------------------------
  gboolean Message::IsWaitingToBeSent ()
  {
    return _waiting_to_be_sent;
  }

  // --------------------------------------------------------------------------------
  void Message::SetPassPhrase (const gchar *passphrase)
  {
    g_free (_passphrase);
    _passphrase = g_strdup (passphrase);
  }

  // --------------------------------------------------------------------------------
  const gchar *Message::GetPassPhrase ()
  {
    return _passphrase;
  }

  // --------------------------------------------------------------------------------
  void Message::SetFitness (const guint value)
  {
    g_key_file_set_integer (_key_file,
                            "Header",
                            "fitness",
                            value);
    _is_valid = TRUE;
  }

  // --------------------------------------------------------------------------------
  guint Message::GetFitness ()
  {
    return g_key_file_get_integer (_key_file,
                                   "Header",
                                   "fitness",
                                   NULL);
  }

  // --------------------------------------------------------------------------------
  void Message::Spread ()
  {
    Ring::SpreadMessage (this);
  }

  // --------------------------------------------------------------------------------
  void Message::MarkAsSent ()
  {
    _waiting_to_be_sent = FALSE;
  }

  // --------------------------------------------------------------------------------
  void Message::MarkAsWaitingToBeSent ()
  {
    _waiting_to_be_sent = TRUE;
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
  void Message::Remove (const gchar *field)
  {
    GError *error = NULL;

    if (g_key_file_remove_key (_key_file,
                               "Body",
                               field,
                               &error) == FALSE)
    {
      g_warning ("Message::Remove: %s", error->message);
      g_clear_error (&error);
    }
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
