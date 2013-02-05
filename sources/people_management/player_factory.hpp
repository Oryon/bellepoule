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

#ifndef player_factory_hpp
#define player_factory_hpp

#include "common/player.hpp"

class PlayerFactory
{
  public:
    typedef Player *(*Constructor) ();

    static void AddPlayerType (const gchar *player_type,
                               Constructor  constructor);

    static Player *CreatePlayer (const gchar *player_type);

  private:
    struct PlayerClass
    {
      const gchar *_player_type;
      Constructor  _constructor;
    };

    static GSList *_classes;

  private:
    PlayerFactory ();
    virtual ~PlayerFactory ();
};

#endif
