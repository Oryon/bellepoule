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

#include <libxml/encoding.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "xml_scheme.hpp"
#include "data.hpp"

// --------------------------------------------------------------------------------
Data::Data (const gchar *xml_name,
            guint        default_value)
: Object ("Data")
{
  _xml_name = g_strdup (xml_name);
  _value    = default_value;
  _valid    = TRUE;
  _string   = nullptr;

  _is_integer = TRUE;
}

// --------------------------------------------------------------------------------
Data::Data (const gchar *xml_name,
            const gchar *default_value)
: Object ("Data")
{
  _xml_name = g_strdup (xml_name);
  _string   = g_strdup (default_value);
  _valid    = TRUE;

  _is_integer = FALSE;
}

// --------------------------------------------------------------------------------
Data::~Data ()
{
  g_free (_xml_name);
  g_free (_string);
}

// --------------------------------------------------------------------------------
void Data::SetValue (guint value)
{
  _value = value;
}

// --------------------------------------------------------------------------------
guint Data::GetValue ()
{
  return _value;
}

// --------------------------------------------------------------------------------
void Data::Copy (Data *from)
{
  if (_is_integer)
  {
    _value = from->_value;
  }
  else
  {
    SetString (from->GetString ());
  }
}

// --------------------------------------------------------------------------------
void Data::SetString (const char *string)
{
  g_free (_string);
  _string = g_strdup (string);
}

// --------------------------------------------------------------------------------
const gchar *Data::GetString ()
{
  return _string;
}

// --------------------------------------------------------------------------------
gboolean Data::Load (xmlNode *xml_node)
{
  gchar *prop = (gchar *) xmlGetProp (xml_node,
                                      BAD_CAST _xml_name);

  SetString (prop);

  if (_string)
  {
    _value = (guint) atoi (_string);
    _valid = TRUE;
    xmlFree (prop);
    return TRUE;
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
void Data::Save (XmlScheme *xml_scheme)
{
  if (_valid)
  {
    if (_is_integer)
    {
      xml_scheme->WriteFormatAttribute (_xml_name, "%d", _value);
    }
    else if (_string)
    {
      xml_scheme->WriteAttribute (_xml_name, _string);
    }
  }
}

// --------------------------------------------------------------------------------
void Data::Activate ()
{
  _valid = TRUE;
}

// --------------------------------------------------------------------------------
void Data::Deactivate ()
{
  _valid = FALSE;
}

// --------------------------------------------------------------------------------
gboolean Data::IsValid ()
{
  return _valid;
}
