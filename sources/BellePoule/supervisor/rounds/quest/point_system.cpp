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

#include "duel_score.hpp"
#include "elo.hpp"
#include "point_system.hpp"

namespace Quest
{
  // --------------------------------------------------------------------------------
  PointSystem::PointSystem (Stage *owner)
    : Object ("PointSystem")
  {
    _owner   = owner;
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

  // --------------------------------------------------------------------------------
  gint PointSystem::Compare (Player *A,
                             Player *B)
  {
    Player::AttributeId attr_id ("");
    gint                result;

    {
      guint      duel_scoreA      = 0;
      guint      duel_scoreB      = 0;
      Attribute *duel_scoreA_attr;
      Attribute *duel_scoreB_attr;


      attr_id._name = (gchar *) "score_quest";
      duel_scoreA_attr = A->GetAttribute (&attr_id);
      duel_scoreB_attr = B->GetAttribute (&attr_id);

      if (duel_scoreA_attr)
      {
        duel_scoreA = duel_scoreA_attr->GetUIntValue ();
      }
      if (duel_scoreB_attr)
      {
        duel_scoreB = duel_scoreB_attr->GetUIntValue ();
      }

      result = duel_scoreB - duel_scoreA;
      if (result)
      {
        return result;
      }
    }

    {
      guint eloA;
      guint eloB;

      attr_id._name = (gchar *) "elo";
      eloA = A->GetAttribute (&attr_id)->GetUIntValue ();
      eloB = B->GetAttribute (&attr_id)->GetUIntValue ();

      result = eloB - eloA;
      if (result)
      {
        return result;
      }
    }

    return Player::RandomCompare (A,
                                  B,
                                  _owner->GetRandSeed ());
  }

}
