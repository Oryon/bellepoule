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

GSList *PlayerFactory::_classes = NULL;

// --------------------------------------------------------------------------------
PlayerFactory::PlayerFactory ()
{
}

// --------------------------------------------------------------------------------
PlayerFactory::~PlayerFactory ()
{
}

// --------------------------------------------------------------------------------
void PlayerFactory::AddPlayerType (const gchar *player_type,
                                   Constructor  constructor)
{
  PlayerClass *new_class = new PlayerClass;

  new_class->_player_type = player_type;
  new_class->_constructor = constructor;

  _classes = g_slist_prepend (_classes,
                              new_class);
}

// --------------------------------------------------------------------------------
Player *PlayerFactory::CreatePlayer (const gchar *player_type)
{
  GSList *current = _classes;

  while (current)
  {
    PlayerClass *player_class = (PlayerClass *) current->data;

    if (g_ascii_strcasecmp (player_class->_player_type, player_type) == 0)
    {
      return player_class->_constructor ();
    }
    current = g_slist_next (current);
  }

  return NULL;
}
