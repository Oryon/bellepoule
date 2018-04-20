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

#include "util/attribute.hpp"

#include "player_factory.hpp"
#include "referee.hpp"

const gchar *Referee::_class_name = "Referee";
const gchar *Referee::_xml_tag    = "Arbitre";

// --------------------------------------------------------------------------------
Referee::Referee ()
: Player (_class_name)
{
  Player::AttributeId attr_id ("");

  attr_id._name = (gchar *) "connection";
  SetAttributeValue (&attr_id, "Manual");
}

// --------------------------------------------------------------------------------
Referee::~Referee ()
{
}

// --------------------------------------------------------------------------------
Player *Referee::Clone ()
{
  return new Referee ();
}

// --------------------------------------------------------------------------------
void Referee::RegisterPlayerClass ()
{
  PlayerFactory::AddPlayerClass (_class_name,
                                 _xml_tag,
                                 CreateInstance);
}

// --------------------------------------------------------------------------------
const gchar *Referee::GetXmlTag ()
{
  return _xml_tag;
}

// --------------------------------------------------------------------------------
Player *Referee::CreateInstance ()
{
  return new Referee ();
}

// --------------------------------------------------------------------------------
void Referee::GiveAnId ()
{
  AttributeId  attr_id ("ref");
  Attribute   *attr = GetAttribute (&attr_id);

  if (attr == NULL)
  {
    Player::AttributeId name_attr_id      ("name");
    Player::AttributeId firstname_attr_id ("first_name");
    GChecksum *checksum = g_checksum_new (G_CHECKSUM_SHA1);

    {
      Attribute *name_attr      = GetAttribute (&name_attr_id);
      Attribute *firstname_attr = GetAttribute (&firstname_attr_id);
      gchar     *digest;

      g_checksum_update (checksum,
                         (guchar *) name_attr->GetStrValue (),
                         -1);
      g_checksum_update (checksum,
                         (guchar *) firstname_attr->GetStrValue (),
                         -1);

      digest = g_strdup (g_checksum_get_string (checksum));
      digest[8] = 0;
      SetRef (g_ascii_strtoll (digest,
                               NULL,
                               16));
      g_free (digest);
    }

    g_checksum_free (checksum);
  }
}
