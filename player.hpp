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

#ifndef player_hpp
#define player_hpp

#include <gtk/gtk.h>
#include <libxml/xmlwriter.h>

#include "attribute.hpp"
#include "object.hpp"

class Match_c;
class Player_c : public Object_c
{
  public:
     Player_c ();

    Attribute_c *GetAttribute (gchar *name);

    void SetAttributeValue (gchar *name,
                            gchar *value);

    void SetAttributeValue (gchar *name,
                            guint  value);

    guint  GetRef ();

    void Save (xmlTextWriter *xml_writer);

    void Load (xmlNode *xml_node);

    gchar *GetName ();

    static gint CompareWithRef (Player_c *player,
                                guint     ref);

    static gint Compare (Player_c *a,
                         Player_c *b,
                         gchar    *attr_name);

  private:
    static guint   _next_ref;
    static GSList *_attributes_model;

    GSList *_attributes;
    guint   _ref;

    ~Player_c ();
};

#endif
