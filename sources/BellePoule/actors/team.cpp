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
#include "util/data.hpp"
#include "util/xml_scheme.hpp"

#include "player_factory.hpp"
#include "fencer.hpp"
#include "team.hpp"

const gchar *Team::_class_name = "Team";
const gchar *Team::_xml_tag    = "Equipe";

// --------------------------------------------------------------------------------
Team::Team ()
: Player (_class_name)
{
  _member_list = nullptr;

  _default_classification = nullptr;
  _minimum_size           = nullptr;
  _manual_classification  = nullptr;
  _enable_member_saving   = TRUE;
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
  if (_manual_classification && _minimum_size && _default_classification)
  {
    Player::AttributeId attending_attr_id ("attending");
    Player::AttributeId ranking_attr_id   ("ranking");
    GSList *current;
    guint   present_count = 0;
    guint   team_rank     = 0;

    if (_manual_classification->GetValue () == FALSE)
    {
      _member_list = g_slist_sort_with_data (_member_list,
                                             (GCompareDataFunc) Player::Compare,
                                             &ranking_attr_id);

      SetAttributeValue (&ranking_attr_id,
                         (guint) 0);
    }

    current = _member_list;
    while (current)
    {
      Player    *player    = (Player *) current->data;
      Attribute *attending = player->GetAttribute (&attending_attr_id);

      if (attending && attending->GetUIntValue ())
      {
        present_count++;

        if (present_count <= _minimum_size->GetValue ())
        {
          guint      value = _default_classification->GetValue ();
          Attribute *rank  = player->GetAttribute (&ranking_attr_id);

          if (rank && rank->GetUIntValue ())
          {
            value = rank->GetUIntValue ();
          }
          team_rank += value;
        }
      }
      current = g_slist_next (current);
    }

    SetAttributeValue (&attending_attr_id,
                       (present_count >= _minimum_size->GetValue ()));

    if (_manual_classification->GetValue () == FALSE)
    {
      SetAttributeValue (&ranking_attr_id,
                         team_rank);
    }
  }
}

// --------------------------------------------------------------------------------
void Team::SetAttributesFromMembers ()
{
  GList *current_desc = AttributeDesc::GetList ();

  while (current_desc)
  {
    AttributeDesc *attr_desc = (AttributeDesc *) current_desc->data;

    if (   (attr_desc->_uniqueness  == AttributeDesc::Uniqueness::NOT_SINGULAR)
        && (attr_desc->_persistency == AttributeDesc::Persistency::PERSISTENT)
        && (g_strcmp0 (attr_desc->_code_name, "team") != 0))
    {
      AttributeId  attr_id (attr_desc->_code_name);
      GSList      *current_member = _member_list;
      Attribute   *team_attr      = nullptr;

      RemoveAttribute (&attr_id);

      while (current_member)
      {
        Player    *player = (Player *) current_member->data;
        Attribute *attr   = player->GetAttribute (&attr_id);

        if (team_attr && (Attribute::Compare (team_attr, attr) != 0))
        {
          team_attr = nullptr;
          break;
        }
        team_attr = attr;

        current_member = g_slist_next (current_member);
      }

      if (team_attr)
      {
        SetAttribute (team_attr);
      }
    }

    current_desc = g_list_next (current_desc);
  }
}

// --------------------------------------------------------------------------------
void Team::SetDefaultClassification (Data *default_classification)
{
  _default_classification = default_classification;
}

// --------------------------------------------------------------------------------
void Team::SetMinimumSize (Data *size)
{
  _minimum_size = size;
}

// --------------------------------------------------------------------------------
void Team::SetManualClassification (Data *manual)
{
  _manual_classification = manual;
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
void Team::EnableMemberSaving (gboolean enable)
{
  _enable_member_saving = enable;
}

// --------------------------------------------------------------------------------
void Team::Load (xmlNode *xml_node)
{
  Player::Load (xml_node);

  {
    Player::AttributeId  name_attr_id ("name");
    Attribute           *name_attr = GetAttribute (&name_attr_id);

    if (   (name_attr == nullptr)
        || (g_strcmp0 (name_attr->GetStrValue (), "???") == 0))
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
        }
      }
    }
  }
}

// --------------------------------------------------------------------------------
void Team::SaveMembers (XmlScheme *xml_scheme)
{
  if (_enable_member_saving)
  {
    GSList *current_member = _member_list;

    while (current_member)
    {
      Player *player = (Player *) current_member->data;

      player->Save (xml_scheme);

      current_member = g_slist_next (current_member);
    }
  }
}

// --------------------------------------------------------------------------------
void Team::Save (XmlScheme *xml_scheme,
                 gboolean   full_profile)
{
  xml_scheme->StartElement (GetXmlTag ());

  SaveAttributes (xml_scheme);
  SaveMembers (xml_scheme);

  xml_scheme->EndElement ();
}
