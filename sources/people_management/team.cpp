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

#include "player_factory.hpp"
#include "team.hpp"

const gchar *Team::_class_name = "Team";
const gchar *Team::_xml_tag    = "Equipe";

// --------------------------------------------------------------------------------
Team::Team ()
: Player (_class_name)
{
  _member_list = NULL;
}

// --------------------------------------------------------------------------------
Team::~Team ()
{
  g_slist_free (_member_list);
}

// --------------------------------------------------------------------------------
Player *Team::Clone ()
{
  return new Team ();
}

// --------------------------------------------------------------------------------
void Team::SetAttendingFromMembers ()
{
  Player::AttributeId  attending_attr_id ("attending");
  GSList *current       = _member_list;
  guint   present_count = 0;

  while (current)
  {
    Player    *player    = (Player *) current->data;
    Attribute *attending = player->GetAttribute (&attending_attr_id);

    if (attending && attending->GetUIntValue ())
    {
      present_count++;
    }
    current = g_slist_next (current);
  }

  SetAttributeValue (&attending_attr_id,
                     (present_count >= REQUIRED_PLAYER_COUNT));
}

// --------------------------------------------------------------------------------
void Team::AddMember (Player *member)
{
  _member_list = g_slist_prepend (_member_list,
                                  member);
}

// --------------------------------------------------------------------------------
void Team::RemoveMember (Player *member)
{
  _member_list = g_slist_remove (_member_list,
                                  member);
}

// --------------------------------------------------------------------------------
GSList *Team::GetMemberList ()
{
  return _member_list;
}

// --------------------------------------------------------------------------------
void Team::RegisterPlayerClass ()
{
  PlayerFactory::AddPlayerClass (_class_name,
                                 _xml_tag,
                                 CreateInstance);
}

// --------------------------------------------------------------------------------
const gchar *Team::GetXmlTag ()
{
  return _xml_tag;
}

// --------------------------------------------------------------------------------
Player *Team::CreateInstance ()
{
  return new Team ();
}

// --------------------------------------------------------------------------------
void Team::Load (xmlNode *xml_node)
{
  Player::Load (xml_node);

  {
    Player::AttributeId  name_attr_id ("name");
    Attribute           *name_attr = GetAttribute (&name_attr_id);

    if (name_attr == NULL)
    {
      Player::AttributeId  ref_attr_id ("ref");
      Attribute           *ref_attr = GetAttribute (&ref_attr_id);

      if (ref_attr)
      {
        AttributeDesc *ref_desc = ref_attr->GetDesc ();
        gchar         *name     = (gchar *) xmlGetProp (xml_node, BAD_CAST ref_desc->_xml_name);

        if (name)
        {
          SetAttributeValue (&name_attr_id,
                             name);

          {
            AttributeDesc *team_desc = AttributeDesc::GetDescFromCodeName ("team");

            team_desc->AddDiscreteValues (name,
                                          name,
                                          NULL,
                                          NULL);
          }
        }
      }
    }
  }
}
