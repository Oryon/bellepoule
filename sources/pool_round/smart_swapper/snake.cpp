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

#include "snake.hpp"

namespace SmartSwapper
{
  // --------------------------------------------------------------------------------
  Snake::Snake (guint length)
    : Object ("SmartSwapper::Snake")
  {
    _length = length;
  }

  // --------------------------------------------------------------------------------
  Snake::~Snake ()
  {
  }

  // --------------------------------------------------------------------------------
  void Snake::Reset (guint position)
  {
    _request_count = 0;
    _odd_cursor    = position;
    _even_cursor   = position;
  }

  // --------------------------------------------------------------------------------
  guint Snake::GetNextEvenCursor ()
  {
    if (_even_cursor > 1)
    {
      _even_cursor--;
      return _even_cursor;
    }
    else
    {
      return GetNextOddCursor ();
    }
  }

  // --------------------------------------------------------------------------------
  guint Snake::GetNextOddCursor ()
  {
    if (_odd_cursor < _length)
    {
      _odd_cursor++;
      return _odd_cursor;
    }
    else
    {
      return GetNextEvenCursor ();
    }
  }

  // --------------------------------------------------------------------------------
  guint Snake::GetNextPosition ()
  {
    _request_count++;

    if (_request_count == 1)
    {
      return _odd_cursor;
    }
    else
    {
      if (_request_count % 2)
      {
        return GetNextEvenCursor ();
      }
      else
      {
        return GetNextOddCursor ();
      }
    }
  }
}
