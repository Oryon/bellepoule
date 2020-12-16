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

#include "util/player.hpp"
#include "util/attribute.hpp"
#include "../../match.hpp"
#include "../../stage.hpp"

#include "elo.hpp"
#include "point_system.hpp"

namespace Generic
{
  // --------------------------------------------------------------------------------
  PointSystem::PointSystem (guint32  rand_seed,
                            gboolean reverse_insertion)
    : Object ("PointSystem")
  {
    _rand_seed         = rand_seed;
    _matches           = nullptr;
    _reverse_insertion = reverse_insertion;

    _elo = new Elo ();
  }

  // --------------------------------------------------------------------------------
  PointSystem::~PointSystem ()
  {
    g_list_free (_matches);

    _elo->Release ();
  }

  // --------------------------------------------------------------------------------
  void PointSystem::RateMatch (Match *match)
  {
    if (g_list_find (_matches,
                     match) == nullptr)
    {
      if (_reverse_insertion)
      {
        _matches = g_list_prepend (_matches,
                                   match);
      }
      else
      {
        _matches = g_list_append (_matches,
                                  match);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void PointSystem::Rehash ()
  {
    _elo->ProcessBatch (_matches);
  }

  // --------------------------------------------------------------------------------
  void PointSystem::Reset ()
  {
    _elo->CancelBatch ();

    g_list_free (_matches);
    _matches = nullptr;
  }

  // --------------------------------------------------------------------------------
  gint PointSystem::Compare (Player *A,
                             Player *B)
  {
    Player::AttributeId attr_id ("");
    guint eloA;
    guint eloB;
    gint  result;

    attr_id._name = (gchar *) "elo";
    eloA = A->GetAttribute (&attr_id)->GetUIntValue ();
    eloB = B->GetAttribute (&attr_id)->GetUIntValue ();

    result = eloB - eloA;
    if (result)
    {
      return result;
    }

    return Player::RandomCompare (A,
                                  B,
                                  _rand_seed);
  }
}
