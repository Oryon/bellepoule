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

#include "opponent.hpp"

namespace Pool
{
  // --------------------------------------------------------------------------------
  Opponent::Opponent (guint id)
    : Object ("Opponent")
  {
    _id            = id;
    _fitness       = 0;
    _opponent_list = NULL;
  }

  // --------------------------------------------------------------------------------
  Opponent::~Opponent ()
  {
    g_list_free (_opponent_list);
  }

  // --------------------------------------------------------------------------------
  guint Opponent::GetId ()
  {
    return _id;
  }

  // --------------------------------------------------------------------------------
  void Opponent::Feed (GList *opponent_list)
  {
    _opponent_list = g_list_copy (opponent_list);

    _opponent_list = g_list_delete_link (_opponent_list,
                                         g_list_nth (_opponent_list, _id-1));
  }

  // --------------------------------------------------------------------------------
  Opponent *Opponent::GetBestOpponent ()
  {
    _opponent_list = g_list_sort (_opponent_list,
                                  (GCompareFunc) CompareFitness);

    if (_opponent_list)
    {
      Opponent *target = (Opponent *) _opponent_list->data;

      _opponent_list = g_list_delete_link (_opponent_list,
                                           _opponent_list);
      target->_opponent_list = g_list_delete_link (target->_opponent_list,
                                                   g_list_find (target->_opponent_list, this));

      return target;
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  void Opponent::SetFitness (guint fitness)
  {
    _fitness = fitness;
  }

  // --------------------------------------------------------------------------------
  gint Opponent::CompareFitness (Opponent *a,
                                 Opponent *b)
  {
    return a->_fitness - b->_fitness;
  }

  // --------------------------------------------------------------------------------
  void Opponent::Dump ()
  {
  }
}
