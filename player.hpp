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
    typedef void (*OnChange) (Player_c    *player,
                              Attribute_c *attr,
                              void        *data);

  public:
     Player_c ();

    Attribute_c *GetAttribute (gchar    *name,
                               Object_c *owner = NULL);

    void SetAttributeValue (gchar    *name,
                            gchar    *value,
                            Object_c *owner = NULL);

    void SetAttributeValue (gchar    *name,
                            guint     value,
                            Object_c *owner = NULL);

    guint GetRef ();

    void Save (xmlTextWriter *xml_writer);

    void Load (xmlNode *xml_node);

    gchar *GetName ();

    void SetChangeCbk (gchar    *attr_name,
                       OnChange  change_cbk,
                       void     *data);

    static gint CompareWithRef (Player_c *player,
                                guint     ref);

    static gint Compare (Player_c *a,
                         Player_c *b,
                         gchar    *attr_name);

  private:
    struct Client : public Object_c
    {
      gchar    *_attr_name;
      OnChange  _change_cbk;
      void     *_data;
    };

    GSList *_clients;

    static guint   _next_ref;
    static GSList *_attributes_model;

    GSList *_attributes;
    guint   _ref;

    ~Player_c ();

    void NotifyChange (Attribute_c *attr);
};

#endif
