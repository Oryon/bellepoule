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

GSList *PlayerFactory::_classes = nullptr;

// --------------------------------------------------------------------------------
PlayerFactory::PlayerFactory ()
{
}

// --------------------------------------------------------------------------------
PlayerFactory::~PlayerFactory ()
{
}

// --------------------------------------------------------------------------------
void PlayerFactory::AddPlayerClass (const gchar *class_name,
                                    const gchar *xml_tag,
                                    Constructor  constructor)
{
  PlayerClass *new_class = new PlayerClass;

  new_class->_class_name  = class_name;
  new_class->_xml_tag     = xml_tag;
  new_class->_constructor = constructor;

  _classes = g_slist_prepend (_classes,
                              new_class);
}

// --------------------------------------------------------------------------------
PlayerFactory::PlayerClass *PlayerFactory::GetPlayerClass (const gchar *class_name)
{
  GSList *current = _classes;

  while (current)
  {
    PlayerClass *current_player_class = (PlayerClass *) current->data;

    if (g_ascii_strcasecmp (current_player_class->_class_name, class_name) == 0)
    {
      return current_player_class;
    }
    current = g_slist_next (current);
  }

  return nullptr;
}

// --------------------------------------------------------------------------------
Player *PlayerFactory::CreatePlayer (const gchar *class_name)
{
  PlayerClass *class_desc = GetPlayerClass (class_name);

  if (class_desc)
  {
    return class_desc->_constructor ();
  }

  return nullptr;
}

// --------------------------------------------------------------------------------
const gchar *PlayerFactory::GetXmlTag (const gchar *class_name)
{
  PlayerClass *class_desc = GetPlayerClass (class_name);

  if (class_desc)
  {
    return class_desc->_xml_tag;
  }

  return nullptr;
}
