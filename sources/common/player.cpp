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

#include "util/attribute.hpp"
#include "network/cryptor.hpp"

#include "player.hpp"

guint Player::_next_ref = 0;

// --------------------------------------------------------------------------------
Player::Player (const gchar *player_class)
  : Object ("Player")
{
  _ref = _next_ref;
  _next_ref++;

  _nb_matchs = 0;

  _player_class = player_class;

  _clients = NULL;

  _wifi_code = new WifiCode (this);

  {
    Player::AttributeId attr_id ("");

    attr_id._name = (gchar *) "attending";
    SetAttributeValue (&attr_id, (guint) FALSE);

    attr_id._name = (gchar *) "exported";
    SetAttributeValue (&attr_id, (guint) FALSE);

    attr_id._name = (gchar *) "availability";
    SetAttributeValue (&attr_id, "Absent");

    attr_id._name = (gchar *) "global_status";
    SetAttributeValue (&attr_id, "F");
  }
}

// --------------------------------------------------------------------------------
Player::~Player ()
{
  _wifi_code->Release ();

  g_slist_foreach (_clients,
                   (GFunc) Object::TryToRelease,
                   NULL);
  g_slist_free (_clients);
}

// --------------------------------------------------------------------------------
gboolean Player::Is (const gchar *player_class)
{
  return (g_ascii_strcasecmp (_player_class, player_class) == 0);
}

// --------------------------------------------------------------------------------
Player *Player::Duplicate ()
{
  Player *player       = Clone ();
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
Player::AttributeId *Player::AttributeId::Create (AttributeDesc *desc,
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
                           Object      *owner,
                           guint        steps)
{
  Client *client = new Client;

  client->_attr_name  = g_strdup (attr_name);
  client->_change_cbk = change_cbk;
  client->_owner      = owner;
  client->_steps      = steps;

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

    if (client->_owner == owner)
    {
      remove_list = g_slist_prepend (remove_list,
                                     client);
    }
    current = g_slist_next (current);
  }

  current = remove_list;
  while (current)
  {
    Client *client = (Client *) current->data;

    client->Release ();
    _clients = g_slist_remove (_clients,
                               current->data);
    current = g_slist_next (current);
  }

  g_slist_free (remove_list);
}

// --------------------------------------------------------------------------------
void Player::NotifyChange (const gchar *attr_name)
{
  AttributeId  id (attr_name);
  Attribute   *attr = GetAttribute (&id);

  if (attr)
  {
    NotifyChange (attr, BEFORE_CHANGE);
    NotifyChange (attr, AFTER_CHANGE);
  }
}

// --------------------------------------------------------------------------------
void Player::NotifyChange (Attribute *attr,
                           guint      step)
{
  GSList *list = _clients;

  while (list)
  {
    Client *client;

    client = (Client *) list->data;
    if (   (client->_steps & step)
        && (strcmp (client->_attr_name, attr->GetCodeName ()) == 0))
    {
      client->_change_cbk (this,
                           attr,
                           client->_owner,
                           step);
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
  else if ((attr->GetStrValue () == NULL) && (value == NULL))
  {
    return;
  }
  else if (attr->GetStrValue () && value && strcmp (attr->GetStrValue (), value) == 0)
  {
    return;
  }

  NotifyChange (attr, BEFORE_CHANGE);
  attr->SetValue (value);
  NotifyChange (attr, AFTER_CHANGE);
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

  NotifyChange (attr, BEFORE_CHANGE);
  attr->SetValue (value);
  NotifyChange (attr, AFTER_CHANGE);
}

// --------------------------------------------------------------------------------
void Player::SetAttribute (Attribute *attr)
{
  AttributeId attr_id (attr->GetCodeName ());

  RemoveAttribute (&attr_id);

  SetData (attr_id._owner,
           attr_id._name,
           attr->Duplicate (),
           (GDestroyNotify) Object::TryToRelease);
}

// --------------------------------------------------------------------------------
void Player::RemoveAttribute (AttributeId *attr_id)
{
  RemoveData (attr_id->_owner,
              attr_id->_name);
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
gchar Player::GetWeaponCode ()
{
  return _weapon;
}

// --------------------------------------------------------------------------------
void Player::SetWeaponCode (gchar weapon)
{
  _weapon = weapon;
}

// --------------------------------------------------------------------------------
void Player::AddMatchs (guint nb_matchs)
{
  _nb_matchs += nb_matchs;
}

// --------------------------------------------------------------------------------
void Player::RemoveMatchs (guint nb_matchs)
{
  _nb_matchs -= nb_matchs;
}

// --------------------------------------------------------------------------------
guint Player::GetNbMatchs ()
{
  return _nb_matchs;
}

// --------------------------------------------------------------------------------
void Player::SaveAttributes (xmlTextWriter *xml_writer)
{
  GSList *attr_list;
  GSList *current;

  AttributeDesc::CreateExcludingList (&attr_list,
                                      NULL);

  current = attr_list;
  while (current)
  {
    AttributeDesc *desc = (AttributeDesc *) current->data;

    if (   (desc->_persistency == AttributeDesc::PERSISTENT)
        && (desc->_scope       == AttributeDesc::GLOBAL))
    {
      gboolean saving_allowed = TRUE;

      if (strcmp (desc->_code_name, "final_rank") == 0)
      {
        AttributeId  exported_id ("exported");
        Attribute   *exported_attr = GetAttribute (&exported_id);

        saving_allowed = exported_attr->GetUIntValue () == FALSE;
      }

      if (saving_allowed)
      {
        AttributeId  attr_id (desc->_code_name);
        Attribute   *attr = GetAttribute (&attr_id);

        if (attr)
        {
          gchar *xml_image = attr->GetXmlImage ();

          if (strcmp (desc->_code_name, "birth_date") == 0)
          {
            GDate date;

            g_date_set_parse (&date,
                              attr->GetStrValue ());

            if (g_date_valid (&date))
            {
              gchar buffer[50];

              g_date_strftime (buffer,
                               sizeof (buffer),
                               "%d.%m.%Y",
                               &date);
              g_free (xml_image);
              xml_image = g_strdup (buffer);
            }
          }

          xmlTextWriterWriteAttribute (xml_writer,
                                       BAD_CAST attr->GetXmlName (),
                                       BAD_CAST xml_image);
          g_free (xml_image);
        }
      }
    }
    current = g_slist_next (current);
  }

  g_slist_free (attr_list);
}

// --------------------------------------------------------------------------------
void Player::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST GetXmlTag ());

  SaveAttributes (xml_writer);

  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void Player::Load (xmlNode *xml_node)
{
  GSList      *attr_list;
  GSList      *current;
  AttributeId attending_attr_id ("attending");

  AttributeDesc::CreateExcludingList (&attr_list,
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

        if (strcmp (desc->_code_name, "birth_date") == 0)
        {
          Attribute *attr = GetAttribute (&attr_id);

          if (attr)
          {
            gchar *french_date = attr->GetStrValue ();
            gchar **splitted_date;

            splitted_date = g_strsplit_set (french_date,
                                            ".",
                                            0);
            if (   splitted_date
                && splitted_date[0]
                && splitted_date[1]
                && splitted_date[2])
            {
              gchar  buffer[50];
              GDate *date = g_date_new ();

              g_date_set_day   (date, (GDateDay)   atoi (splitted_date[0]));
              g_date_set_month (date, (GDateMonth) atoi (splitted_date[1]));
              g_date_set_year  (date, (GDateYear)  atoi (splitted_date[2]));

              g_date_strftime (buffer,
                               sizeof (buffer),
                               "%x",
                               date);
              attr->SetValue (buffer);

              g_date_free (date);
            }
            g_strfreev (splitted_date);
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
        xmlFree (value);
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

  return attr->GetUserImage (AttributeDesc::LONG_TEXT);
}

// --------------------------------------------------------------------------------
void Player::SetName (const gchar *name)
{
  Player::AttributeId  attr_id ("name");

  SetAttributeValue (&attr_id,
                     name);
}

// --------------------------------------------------------------------------------
gboolean Player::SendMessage (const gchar *where,
                              const gchar *message)
{
  Player::AttributeId  attr_id ("IP");
  Attribute           *ip_attr = GetAttribute (&attr_id);

  if (ip_attr)
  {
    gchar *ip = ip_attr->GetStrValue ();

    if (ip && (ip[0] != 0))
    {
      Net::Uploader *uploader;

      {
        gchar *url;

        if (strchr (ip, ':'))
        {
          url = g_strdup_printf ("http://%s", ip);
        }
        else
        {
          url = g_strdup_printf ("http://%s:35831", ip);
        }

        uploader = new Net::Uploader (url,
                                      (Net::Uploader::UploadStatus) OnUploaderStatus, this,
                                      NULL, NULL);

        g_free (url);
      }

      {
        gchar        *encrypted_msg;
        gchar        *secret_key;
        guchar       *iv;
        Net::Cryptor *cryptor       = new Net::Cryptor ();
        gchar        *full_message  = g_strdup_printf ("%s/%s?ref=%d\n"
                                                       "%s",
                                                       where, _player_class, GetRef (),
                                                       message);

        {
          WifiCode *wifi_code = (WifiCode *) GetFlashCode ();

          secret_key = wifi_code->GetKey ();
        }

        encrypted_msg = cryptor->Encrypt (full_message,
                                          secret_key,
                                          &iv);
        uploader->UploadString (encrypted_msg,
                                iv);

        g_free (full_message);
        g_free (encrypted_msg);
        g_free (secret_key);
        g_free (iv);
        cryptor->Release ();
      }

      uploader->Release ();
      return TRUE;
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
void Player::OnUploaderStatus (Net::Uploader::PeerStatus  peer_status,
                               Object                    *object)
{
  Player              *player = dynamic_cast <Player *> (object);
  Player::AttributeId  connection_attr_id ("connection");

  if (peer_status == Net::Uploader::CONN_ERROR)
  {
    player->SetAttributeValue (&connection_attr_id,
                               "Broken");
  }
  else
  {
    player->SetAttributeValue (&connection_attr_id,
                               "Waiting");
  }
}

// --------------------------------------------------------------------------------
void Player::Dump ()
{
  g_print ("<< %s >>\n", GetName ());
}

// --------------------------------------------------------------------------------
FlashCode *Player::GetFlashCode ()
{
  return _wifi_code;
}
