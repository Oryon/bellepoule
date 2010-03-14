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

class Player : public Object
{
  public:
    class AttributeId : public Object
    {
      public:
        AttributeId (gchar  *name,
                     Object *owner = NULL)
        {
          _name  = name;
          _owner = owner;
        }

        static AttributeId *CreateAttributeId (AttributeDesc *desc,
                                               Object        *owner);

        gchar  *_name;
        Object *_owner;
    };

    typedef void (*OnChange) (Player    *player,
                              Attribute *attr,
                              void      *data);

  public:
    Player ();

    Attribute *GetAttribute (AttributeId *attr_id);

    void SetAttributeValue (AttributeId *attr_id,
                            gchar       *value);

    void SetAttributeValue (AttributeId *attr_id,
                            guint        value);

    guint GetRef ();

    void Save (xmlTextWriter *xml_writer);

    void Load (xmlNode *xml_node);

    gchar *GetName ();

    void SetChangeCbk (gchar    *attr_name,
                       OnChange  change_cbk,
                       void     *data);

    static gint CompareWithRef (Player *player,
                                guint   ref);

    static gint Compare (Player      *a,
                         Player      *b,
                         AttributeId *attr_id);

  private:
    struct Client : public Object
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

    ~Player ();

    void NotifyChange (Attribute *attr);
};

#endif
