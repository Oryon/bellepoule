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

#include "list_crawler.hpp"

namespace NeoSwapper
{
  // --------------------------------------------------------------------------------
  ListCrawler::ListCrawler (GList *list)
    : Object ("SmartSwapper::ListCrawler")
  {
    _list         = list;
    _left_cursor  = nullptr;
    _right_cursor = nullptr;

    _request_count = 0;
  }

  // --------------------------------------------------------------------------------
  ListCrawler::~ListCrawler ()
  {
  }

  // --------------------------------------------------------------------------------
  void ListCrawler::Reset (guint position)
  {
    _request_count = 0;

    _left_cursor  = g_list_nth (_list, position);
    _right_cursor = _left_cursor;

    _left_cursor  = g_list_previous (_left_cursor);
    _right_cursor = g_list_next (_right_cursor);
  }

  // --------------------------------------------------------------------------------
  GList *ListCrawler::GetNext ()
  {
    GList *next;

    if ((_left_cursor == nullptr) && (_right_cursor == nullptr))
    {
      next = nullptr;
    }
    else if (_left_cursor == nullptr)
    {
      next = _right_cursor;
      _right_cursor = g_list_next (_right_cursor);
    }
    else if (_right_cursor == nullptr)
    {
      next = _left_cursor;
      _left_cursor = g_list_previous (_left_cursor);
    }
    else
    {
      _request_count++;

      if (_request_count % 2)
      {
        next = _right_cursor;
        _right_cursor = g_list_next (_right_cursor);
      }
      else
      {
        next = _left_cursor;
        _left_cursor = g_list_previous (_left_cursor);
      }
    }

    return next;
  }
}
