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

#pragma once

#include <gtk/gtk.h>
#include <libxml/xmlwriter.h>

#include "object.hpp"

class Data : public Object
{
  public:
    guint  _value;

    Data (const gchar *xml_name,
          guint        default_value);

    Data (const gchar *xml_name,
          const gchar *default_value);

    void SetString (const char *string);

    const gchar *GetString ();

    void Save (xmlTextWriter *xml_writer);

    gboolean Load (xmlNode *xml_node);

    void Activate ();

    void Deactivate ();

    gboolean IsValid ();

    void Copy (Data *from);

  private:
    gchar    *_xml_name;
    gboolean  _is_integer;
    gboolean  _valid;
    gchar    *_string;

    virtual ~Data ();
};
