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

#include <stdio.h>

#include "field.hpp"

namespace Oauth
{
  // --------------------------------------------------------------------------------
  Field::Field (const gchar *key,
                const gchar *value)
    : Object ("Oauth::Field")
  {
    _key           = g_strdup (key);
    _encoded_key   = g_uri_escape_string (key, nullptr, FALSE);

    _value         = g_strdup (value);
    _encoded_value = g_uri_escape_string (value, nullptr, FALSE);

    _parcel        = g_strdup_printf ("%s=%s",     _encoded_key, _encoded_value);
    _quoted_parcel = g_strdup_printf ("%s=\"%s\"", _encoded_key, _encoded_value);

    _header_form = g_strdup_printf ("%s: %s", _key, _value);
  }

  // --------------------------------------------------------------------------------
  Field::~Field ()
  {
    g_free (_key);
    g_free (_encoded_key);

    g_free (_value);
    g_free (_encoded_value);

    g_free (_parcel);
    g_free (_quoted_parcel);
    g_free (_header_form);
  }

  // --------------------------------------------------------------------------------
  const gchar *Field::GetParcel ()
  {
    return _parcel;
  }

  // --------------------------------------------------------------------------------
  const gchar *Field::GetQuotedParcel ()
  {
    return _quoted_parcel;
  }

  // --------------------------------------------------------------------------------
  const gchar *Field::GetHeaderForm ()
  {
    return _header_form;
  }

  // --------------------------------------------------------------------------------
  void Field::Dump ()
  {
    printf ("%s ===> %s\n", _key, _value);
  }

  // --------------------------------------------------------------------------------
  gint Field::Compare (Field *a,
                       Field *b)
  {
    return g_strcmp0 (a->_encoded_key,
                      b->_encoded_key);
  }
}
