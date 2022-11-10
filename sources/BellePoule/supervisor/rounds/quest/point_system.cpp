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
#include "point_system.hpp"

namespace Quest
{
  // --------------------------------------------------------------------------------
  PointSystem::PointSystem (Stage    *stage,
                            gboolean  reverse_insertion)
    : Object ("Quest::PointSystem"),
      Generic::PointSystem (stage, true /* elo_matters */, reverse_insertion)
  {
    Module *module = dynamic_cast <Module *> (stage);

    _owner      = module->GetDataOwner ();
    _duel_score = new DuelScore (_owner);
  }

  // --------------------------------------------------------------------------------
  PointSystem::~PointSystem ()
  {
    _duel_score->Release ();
  }

  // --------------------------------------------------------------------------------
  void PointSystem::RateMatch (Match *match)
  {
    _duel_score->RateMatch (match);

    Generic::PointSystem::RateMatch (match);
  }

  // --------------------------------------------------------------------------------
  void PointSystem::Reset ()
  {
    for (GList *m = _matches; m; m = g_list_next (m))
    {
      Match *match = (Match *) m->data;

      _duel_score->CancelMatch (match);
    }

    Generic::PointSystem::Reset ();
  }

  // --------------------------------------------------------------------------------
  gint PointSystem::Compare (Player *A,
                             Player *B)
  {
    gint result;

    {
      Player::AttributeId attr_id ("", _owner);

      attr_id._name = (gchar *) "score_quest";
      Attribute *duel_scoreA_attr = A->GetAttribute (&attr_id);
      Attribute *duel_scoreB_attr = B->GetAttribute (&attr_id);
      guint duel_scoreA = duel_scoreA_attr ? duel_scoreA_attr->GetUIntValue () : 0;
      guint duel_scoreB = duel_scoreB_attr ? duel_scoreB_attr->GetUIntValue () : 0;

      result = duel_scoreB - duel_scoreA;
      if (result)
      {
        return result;
      }

      attr_id._name = (gchar *) "victories_count";
      Attribute *victoriesA_attr = A->GetAttribute (&attr_id);
      Attribute *victoriesB_attr = B->GetAttribute (&attr_id);
      guint victoriesA = victoriesA_attr ? victoriesA_attr->GetUIntValue () : 0;
      guint victoriesB = victoriesB_attr ? victoriesB_attr->GetUIntValue () : 0;

      result = victoriesB - victoriesA;
      if (result)
      {
        return result;
      }
    }

    return Generic::PointSystem::Compare (A,
                                          B);
  }
}
