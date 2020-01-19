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

#include "duel_score.hpp"
#include "elo.hpp"
#include "point_system.hpp"

namespace Quest
{
  // --------------------------------------------------------------------------------
  PointSystem::PointSystem (Object *owner)
    : Object ("PointSystem")
  {
    _matches = nullptr;

    _duel_score = new DuelScore (owner);
    _elo   = new Elo ();
  }

  // --------------------------------------------------------------------------------
  PointSystem::~PointSystem ()
  {
    g_list_free (_matches);

    _duel_score->Release ();
    _elo->Release        ();
  }

  // --------------------------------------------------------------------------------
  void PointSystem::AuditMatch (Match *match)
  {
    if (match->IsOver ())
    {
      Player *winner = match->GetWinner ();
      Player *looser = match->GetLooser ();

      if (winner && looser)
      {
        _matches = g_list_prepend (_matches,
                                   match);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void PointSystem::SumUp ()
  {
    for (GList *m = _matches; m; m = g_list_next (m))
    {
      Match *match = (Match *) m->data;

      _duel_score->EvaluateMatch (match);
    }

    _elo->ProcessBatch (_matches);

    g_list_free (_matches);
    _matches = nullptr;
  }
}
