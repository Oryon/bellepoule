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

#include "wheel_of_fortune.hpp"

namespace Quest
{
  // --------------------------------------------------------------------------------
  WheelOfFortune::WheelOfFortune (GSList *list,
                                  guint   rank_seed)
    : Object ("Quest::WheelOfFortune")
  {
     _list       = list;
     _origin     = list;
     _size       = g_slist_length (list);
     _randomizer = g_rand_new_with_seed (rank_seed);
  }

  // --------------------------------------------------------------------------------
  WheelOfFortune::~WheelOfFortune ()
  {
    g_rand_free (_randomizer);
  }

  // --------------------------------------------------------------------------------
  void *WheelOfFortune::Turn ()
  {
    guint32 toss = g_rand_int_range (_randomizer,
                                     0, _size-1);

    _origin = g_slist_nth (_list,
                           toss);
    _position = _origin;

    return _position->data;
  }

  // --------------------------------------------------------------------------------
  void *WheelOfFortune::TryAgain ()
  {
    _position = g_slist_next (_position);

    if (_position == nullptr)
    {
      _position = _list;
    }

    if (_position == _origin)
    {
      return nullptr;
    }

    return _position->data;
  }
}
