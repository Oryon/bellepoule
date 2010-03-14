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

guint Player::_next_ref = 0;

// --------------------------------------------------------------------------------
Player::Player ()
: Object ("Player")
{
  _ref = _next_ref;
  _next_ref++;

  _clients = NULL;
}

// --------------------------------------------------------------------------------
Player::~Player ()
{
  g_slist_foreach (_clients,
                   (GFunc) Object::TryToRelease,
                   NULL);
  g_slist_free (_clients);
}

// --------------------------------------------------------------------------------
Player::AttributeId *Player::AttributeId::CreateAttributeId (AttributeDesc *desc,
                                                             Object        *owner)
{
  if (desc->_scope == AttributeDesc::GLOBAL)
  {
    return new Player::AttributeId (desc->_xml_name);
  }
  else
  {
    return new Player::AttributeId (desc->_xml_name, owner);
  }
}

// --------------------------------------------------------------------------------
gint Player::Compare (Player      *a,
                      Player      *b,
                      AttributeId *attr_id)
{
  Attribute *attr_a = a->GetAttribute (attr_id);
  Attribute *attr_b = b->GetAttribute (attr_id);

  return Attribute::Compare (attr_a, attr_b);
}

// --------------------------------------------------------------------------------
gint Player::CompareWithRef (Player *player,
                             guint   ref)
{
  return player->_ref - ref;
}

// --------------------------------------------------------------------------------
Attribute *Player::GetAttribute (AttributeId *attr_id)
{
  return (Attribute*) GetData (attr_id->_owner,
                               attr_id->_name);
}

// --------------------------------------------------------------------------------
void Player::SetChangeCbk (gchar    *attr_name,
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
void Player::NotifyChange (Attribute *attr)
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
void Player::SetAttributeValue (AttributeId *attr_id,
                                gchar       *value)
{
  Attribute *attr = GetAttribute (attr_id);

  if (attr == NULL)
  {
    attr = Attribute::New (attr_id->_name);

    SetData (attr_id->_owner,
             attr_id->_name,
             attr,
             (GDestroyNotify) Object::TryToRelease);
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
void Player::SetAttributeValue (AttributeId *attr_id,
                                guint        value)
{
  Attribute *attr = GetAttribute (attr_id);

  if (attr == NULL)
  {
    attr = Attribute::New (attr_id->_name);

    SetData (attr_id->_owner,
             attr_id->_name,
             attr,
             (GDestroyNotify) Object::TryToRelease);
  }
  else if ((guint) attr->GetValue () == value)
  {
    return;
  }

  attr->SetValue (value);
  NotifyChange (attr);
}

// --------------------------------------------------------------------------------
guint Player::GetRef ()
{
  return _ref;
}

// --------------------------------------------------------------------------------
void Player::Save (xmlTextWriter *xml_writer)
{
  GSList *attr_list;

  AttributeDesc::CreateList (&attr_list,
                             NULL);

  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST "player");

  for (guint i = 0; i < g_slist_length (attr_list); i++)
  {
    AttributeDesc *desc;

    desc = (AttributeDesc *) g_slist_nth_data (attr_list,
                                               i);
    if (desc->_persistency == AttributeDesc::PERSISTENT)
    {
      AttributeId  attr_id (desc->_xml_name);
      Attribute   *attr = GetAttribute (&attr_id);

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
void Player::Load (xmlNode *xml_node)
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
        AttributeId attr_id (desc->_xml_name);

        SetAttributeValue (&attr_id,
                           value);
      }
    }
  }

  {
    Player::AttributeId  attr_id ("ref");
    Attribute           *attr = GetAttribute (&attr_id);

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
gchar *Player::GetName ()
{
  Player::AttributeId  attr_id ("name");
  Attribute           *attr = GetAttribute (&attr_id);

  return attr->GetUserImage ();
}
