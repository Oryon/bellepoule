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

#include <stdlib.h>
#include <string.h>

#include "attribute.hpp"

#include "player.hpp"

guint Player_c::_next_ref = 0;

// --------------------------------------------------------------------------------
Player_c::Player_c ()
: Object_c ("Player_c")
{
  _ref = _next_ref;
  _next_ref++;

  _clients = NULL;
}

// --------------------------------------------------------------------------------
Player_c::~Player_c ()
{
  g_slist_foreach (_clients,
                   (GFunc) Object_c::TryToRelease,
                   NULL);
  g_slist_free (_clients);
}

// --------------------------------------------------------------------------------
gint Player_c::Compare (Player_c *a,
                        Player_c *b,
                        gchar    *attr_name)
{
  Attribute_c *attr_a = a->GetAttribute (attr_name);
  Attribute_c *attr_b = b->GetAttribute (attr_name);

  return Attribute_c::Compare (attr_a, attr_b);
}

// --------------------------------------------------------------------------------
gint Player_c::CompareWithRef (Player_c *player,
                               guint     ref)
{
  return player->_ref - ref;
}

// --------------------------------------------------------------------------------
Attribute_c *Player_c::GetAttribute (gchar    *name,
                                     Object_c *owner)
{
  return (Attribute_c*) GetData (owner,
                                 name);
}

// --------------------------------------------------------------------------------
void Player_c::SetChangeCbk (gchar    *attr_name,
                             OnChange  change_cbk,
                             void     *data)
{
  Client *client = new Client;

  client->_attr_name  = attr_name;
  client->_change_cbk = change_cbk;
  client->_data       = data;

  _clients = g_slist_append (_clients,
                             client);
}

// --------------------------------------------------------------------------------
void Player_c::NotifyChange (Attribute_c *attr)
{
  GSList *list = _clients;

  while (list)
  {
    Client *client;

    client = (Client *) list->data;
    if (strcmp (client->_attr_name, attr->GetName ()) == 0)
    {
      client->_change_cbk (this,
                           attr,
                           client->_data);
    }
    list = g_slist_next (list);
  }
}

// --------------------------------------------------------------------------------
void Player_c::SetAttributeValue (gchar    *name,
                                  gchar    *value,
                                  Object_c *owner)
{
  Attribute_c *attr = GetAttribute (name, owner);

  if (attr == NULL)
  {
    attr = Attribute_c::New (name);

    SetData (owner,
             attr->GetName (),
             attr,
             (GDestroyNotify) Object_c::TryToRelease);
  }

  {
    AttributeDesc *desc       = attr->GetDesc ();
    gchar         *user_image = desc->GetUserImage (value);

    attr->SetValue (user_image);
    g_free (user_image);
  }

  NotifyChange (attr);
}

// --------------------------------------------------------------------------------
void Player_c::SetAttributeValue (gchar    *name,
                                  guint     value,
                                  Object_c *owner)
{
  Attribute_c *attr = GetAttribute (name, owner);

  if (attr == NULL)
  {
    attr = Attribute_c::New (name);

    SetData (owner,
             attr->GetName (),
             attr,
             (GDestroyNotify) Object_c::TryToRelease);
  }
  else if ((guint) attr->GetValue () == value)
  {
    return;
  }

  attr->SetValue (value);
  NotifyChange (attr);
}

// --------------------------------------------------------------------------------
guint Player_c::GetRef ()
{
  return _ref;
}

// --------------------------------------------------------------------------------
void Player_c::Save (xmlTextWriter *xml_writer)
{
  GSList *attr_list;

  AttributeDesc::CreateList (&attr_list,
                             NULL);

  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST "player");

  for (guint i = 0; i < g_slist_length (attr_list ); i++)
  {
    AttributeDesc *desc;

    desc = (AttributeDesc *) g_slist_nth_data (attr_list,
                                               i);
    if (desc->_persistency == AttributeDesc::PERSISTENT)
    {
      Attribute_c *attr = GetAttribute (desc->_xml_name);

      if (attr)
      {
        gchar *xml_image = attr->GetXmlImage ();

        xmlTextWriterWriteFormatAttribute (xml_writer,
                                           BAD_CAST attr->GetName (),
                                           xml_image);
        g_free (xml_image);
      }
    }
  }

  g_slist_free (attr_list);

  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void Player_c::Load (xmlNode *xml_node)
{
  GSList *attr_list;

  AttributeDesc::CreateList (&attr_list,
                             NULL);
  for (guint i = 0; i < g_slist_length (attr_list ); i++)
  {
    AttributeDesc *desc;

    desc = (AttributeDesc *) g_slist_nth_data (attr_list,
                                               i);
    if (desc->_persistency == AttributeDesc::PERSISTENT)
    {
      gchar *value = (gchar *) xmlGetProp (xml_node, BAD_CAST desc->_xml_name);

      if (value)
      {
        SetAttributeValue (desc->_xml_name,
                           value);
      }
    }
  }

  {
    Attribute_c *attr = GetAttribute ("ref");

    if (attr)
    {
      _ref = (guint) attr->GetValue ();
      if (_ref > _next_ref)
      {
        _next_ref = _ref + 1;
      }
    }
  }

  g_slist_free (attr_list);
}

// --------------------------------------------------------------------------------
gchar *Player_c::GetName ()
{
  Attribute_c *attr = GetAttribute ("name");

  return attr->GetUserImage ();
}
