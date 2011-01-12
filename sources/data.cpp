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

#include "data.hpp"

// --------------------------------------------------------------------------------
Data::Data (gchar *xml_name,
            guint  default_value)
{
  _xml_name = xml_name;
  _value    = default_value;

  _is_integer = TRUE;
}

// --------------------------------------------------------------------------------
Data::Data (gchar *xml_name,
            gchar *default_value)
{
  _xml_name = xml_name;
  _string   = default_value;

  _is_integer = FALSE;
}

// --------------------------------------------------------------------------------
Data::~Data ()
{
}

// --------------------------------------------------------------------------------
gboolean Data::Load (xmlNode *xml_node)
{
  _string = (gchar *) xmlGetProp (xml_node,
                                  BAD_CAST _xml_name);
  if (_string)
  {
    _value = (guint) atoi (_string);
    return TRUE;
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
void Data::Save (xmlTextWriter *xml_writer)
{
  if (_is_integer)
  {
    xmlTextWriterWriteFormatAttribute (xml_writer,
                                       BAD_CAST _xml_name,
                                       "%d", _value);
  }
  else if (_string)
  {
    xmlTextWriterWriteFormatAttribute (xml_writer,
                                       BAD_CAST _xml_name,
                                       "%s", _string);
  }
}
