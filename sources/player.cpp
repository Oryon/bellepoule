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
Player::Player (PlayerType player_type)
: Object ("Player")
{
  _ref = _next_ref;
  _next_ref++;

  _player_type = player_type;

  _clients = NULL;

  {
    Player::AttributeId attr_id ("");

    attr_id._name = (gchar *) "attending";
    SetAttributeValue (&attr_id, (guint) FALSE);

    attr_id._name = (gchar *) "exported";
    SetAttributeValue (&attr_id, (guint) FALSE);

    attr_id._name = (gchar *) "availability";
    SetAttributeValue (&attr_id, "Absent");
  }
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
gboolean Player::IsFencer ()
{
  return _player_type == FENCER;
}

// --------------------------------------------------------------------------------
Player *Player::Duplicate ()
{
  Player *player       = new Player (_player_type);
  GSList *current_desc = AttributeDesc::GetList ();

  while (current_desc)
  {
    AttributeDesc *desc;

    desc = (AttributeDesc *) current_desc->data;
    if (desc->_scope == AttributeDesc::GLOBAL)
    {
      AttributeId  attr_id (desc->_code_name);
      Attribute   *attr = GetAttribute (&attr_id);

      if (attr)
      {
        Attribute *new_attr = attr->Duplicate ();

        player->SetData (attr_id._owner,
                         attr_id._name,
                         new_attr,
                         (GDestroyNotify) Object::TryToRelease);
      }
    }

    current_desc = g_slist_next (current_desc);
  }

  return player;
}

// --------------------------------------------------------------------------------
Player::AttributeId *Player::AttributeId::CreateAttributeId (AttributeDesc *desc,
                                                             Object        *owner)
{
  if (desc->_scope == AttributeDesc::GLOBAL)
  {
    return new Player::AttributeId (desc->_code_name);
  }
  else
  {
    return new Player::AttributeId (desc->_code_name, owner);
  }
}

// --------------------------------------------------------------------------------
gint Player::Compare (Player      *a,
                      Player      *b,
                      AttributeId *attr_id)
{
  if (b == NULL)
  {
    return 1;
  }
  else if (a == NULL)
  {
    return -1;
  }
  else
  {
    Attribute *attr_a = a->GetAttribute (attr_id);
    Attribute *attr_b = b->GetAttribute (attr_id);
    gint       result = Attribute::Compare (attr_a, attr_b);

    if ((result == 0) && (attr_id->_rand_seed))
    {
      result = RandomCompare (a,
                              b,
                              attr_id->_rand_seed);
    }

    return result;
  }
}

// --------------------------------------------------------------------------------
gint Player::MultiCompare (Player *a,
                           Player *b,
                           GSList *attr_list)
{
  if (b == NULL)
  {
    return 1;
  }
  else if (a == NULL)
  {
    return -1;
  }
  else
  {
    gint    result;
    GSList *current = attr_list;

    while (current)
    {
      AttributeId *attr_id = (AttributeId *) current->data;
      Attribute   *attr_a  = a->GetAttribute (attr_id);
      Attribute   *attr_b  = b->GetAttribute (attr_id);

      result = Attribute::Compare (attr_a, attr_b);
      if (result)
      {
        break;
      }

      current = g_slist_next (current);
    }

    {
      AttributeId *attr_id = (AttributeId *) attr_list->data;

      if ((result == 0) && (attr_id->_rand_seed))
      {
        result = RandomCompare (a,
                                b,
                                attr_id->_rand_seed);
      }
    }

    return result;
  }
}

// --------------------------------------------------------------------------------
gint Player::RandomCompare (Player  *A,
                            Player  *B,
                            guint32  rand_seed)
{
  guint          ref_A  = A->GetRef ();
  guint          ref_B  = B->GetRef ();
  const guint32  seed[] = {MIN (ref_A, ref_B), MAX (ref_A, ref_B), rand_seed};
  GRand         *rand;
  gint           result;

  // Return always the same random value for the given players
  // to avoid human manipulations. Without that, filling in the
  // same result twice could modify the ranking.
  rand = g_rand_new_with_seed_array (seed,
                                     sizeof (seed) / sizeof (guint32));
  result = (gint) g_rand_int (rand);
  g_rand_free (rand);

  if (ref_A > ref_B)
  {
    return -result;
  }
  else
  {
    return result;
  }
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
  return (Attribute*) GetPtrData (attr_id->_owner,
                                  attr_id->_name);
}

// --------------------------------------------------------------------------------
void Player::SetChangeCbk (const gchar *attr_name,
                           OnChange     change_cbk,
                           Object      *owner)
{
  Client *client = new Client;

  client->_attr_name  = g_strdup (attr_name);
  client->_change_cbk = change_cbk;
  client->_owner      = owner;

  _clients = g_slist_prepend (_clients,
                              client);
}

// --------------------------------------------------------------------------------
void Player::RemoveCbkOwner (Object *owner)
{
  GSList *current;
  GSList *remove_list = NULL;

  current = _clients;
  while (current)
  {
    Client *client = (Client *) current->data;

    if (client->_owner == owner);
    {
      remove_list = g_slist_prepend (remove_list,
                                     client);
      current = g_slist_next (current);
    }
  }

  current = remove_list;
  while (current)
  {
    _clients = g_slist_remove (_clients,
                               current->data);
    current = g_slist_next (current);
  }

  g_slist_free (remove_list);
}

// --------------------------------------------------------------------------------
void Player::NotifyChange (Attribute *attr)
{
  GSList *list = _clients;

  while (list)
  {
    Client *client;

    client = (Client *) list->data;
    if (strcmp (client->_attr_name, attr->GetCodeName ()) == 0)
    {
      client->_change_cbk (this,
                           attr,
                           client->_owner);
    }
    list = g_slist_next (list);
  }
}

// --------------------------------------------------------------------------------
void Player::SetAttributeValue (AttributeId *attr_id,
                                const gchar *value)
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

  attr->SetValue (value);

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
  else if (attr->GetUIntValue () == value)
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
void Player::SetRef (guint ref)
{
  {
    AttributeId attr_id ("ref");

    SetAttributeValue (&attr_id,
                       ref);
  }

  _ref = ref;
}

// --------------------------------------------------------------------------------
void Player::Save (xmlTextWriter *xml_writer,
                   const gchar   *player_tag)
{
  GSList *attr_list;
  GSList *current;

  AttributeDesc::CreateList (&attr_list,
                             NULL);

  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST player_tag);

  current = attr_list;
  while (current)
  {
    AttributeDesc *desc = (AttributeDesc *) current->data;

    if (   (desc->_persistency == AttributeDesc::PERSISTENT)
        && (desc->_scope       == AttributeDesc::GLOBAL))
    {
      AttributeId  attr_id (desc->_code_name);
      Attribute   *attr = GetAttribute (&attr_id);

      if (attr)
      {
        gchar *xml_image = attr->GetXmlImage ();

        xmlTextWriterWriteAttribute (xml_writer,
                                           BAD_CAST attr->GetXmlName (),
                                           BAD_CAST xml_image);
        g_free (xml_image);
      }
    }
    current = g_slist_next (current);
  }

  g_slist_free (attr_list);

  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void Player::Load (xmlNode *xml_node)
{
  GSList      *attr_list;
  GSList      *current;
  AttributeId attending_attr_id ("attending");

  AttributeDesc::CreateList (&attr_list,
                             NULL);

  current = attr_list;
  while (current)
  {
    AttributeDesc *desc = (AttributeDesc *) current->data;

    if (desc->_persistency == AttributeDesc::PERSISTENT)
    {
      gchar *value = (gchar *) xmlGetProp (xml_node, BAD_CAST desc->_xml_name);

      if (value)
      {
        AttributeId attr_id (desc->_code_name);

        SetAttributeValue (&attr_id,
                           value);

        if (strcmp (desc->_code_name, "ref") == 0)
        {
          Attribute *attr = GetAttribute (&attr_id);

          if (attr)
          {
            _ref = attr->GetUIntValue ();
          }
        }

        if (strcmp (desc->_code_name, "global_status") == 0)
        {
          if (value[0] == 'F')
          {
            SetAttributeValue (&attending_attr_id,
                               (guint) FALSE);
          }
          else
          {
            SetAttributeValue (&attending_attr_id,
                               TRUE);
          }
        }
      }
    }
    current = g_slist_next (current);
  }

  if (GetAttribute (&attending_attr_id) == NULL)
  {
    SetAttributeValue (&attending_attr_id,
                       (guint) FALSE);
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
