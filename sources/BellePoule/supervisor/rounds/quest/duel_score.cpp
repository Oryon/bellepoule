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
#include "../../score.hpp"

#include "duel_score.hpp"

namespace Quest
{
  // --------------------------------------------------------------------------------
  DuelScore::DuelScore (Object *owner)
    : Object ("Quest::DuelScore")
  {
    _owner = owner;
  }

  // --------------------------------------------------------------------------------
  DuelScore::~DuelScore ()
  {
  }

  // --------------------------------------------------------------------------------
  void DuelScore::EvaluateMatch (Match *match)
  {
    Player::AttributeId duel_score_attr_id ("score_quest", _owner);

    CancelMatch (match);

    if (match->IsDropped ())
    {
      for (guint i = 0; i < 2; i++)
      {
        Player    *fencer          = match->GetOpponent (i);
        Attribute *duel_score_attr = fencer->GetAttribute (&duel_score_attr_id);

        if (match->GetScore (fencer)->IsOut () == FALSE)
        {
          guint new_duel_score = 2;

          fencer->SetData (match,
                           "Quest::DuelScore",
                           GUINT_TO_POINTER (2));

          if (duel_score_attr)
          {
            new_duel_score += duel_score_attr->GetUIntValue ();
          }
          fencer->SetAttributeValue (&duel_score_attr_id,
                                     new_duel_score);
        }
      }
    }
    else if (match->IsOver ())
    {
      Player *winner = match->GetWinner ();
      Player *looser = match->GetLooser ();

      if (winner && looser)
      {
        guint      winner_score    = match->GetScore (winner)->Get ();
        guint      looser_score    = match->GetScore (looser)->Get ();
        guint      delta           = winner_score - looser_score;
        Attribute *duel_score_attr = winner->GetAttribute (&duel_score_attr_id);
        guint      new_duel_score;

        if (delta < 4)
        {
          new_duel_score = 1;
        }
        else if (4 <= delta && delta <= 7)
        {
          new_duel_score = 2;
        }
        else if (8 <= delta && delta <= 11)
        {
          new_duel_score = 3;
        }
        else
        {
          new_duel_score = 4;
        }

        winner->SetData (match,
                         "Quest::DuelScore",
                         GUINT_TO_POINTER (new_duel_score));

        if (duel_score_attr)
        {
          new_duel_score += duel_score_attr->GetUIntValue ();
        }
        winner->SetAttributeValue (&duel_score_attr_id,
                                   new_duel_score);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void DuelScore::CancelMatch (Match *match)
  {
    Player::AttributeId duel_score_attr_id ("score_quest", _owner);

    for (guint i = 0; i < 2; i++)
    {
      Player *fencer = match->GetOpponent (i);

      if (fencer)
      {
        guint      previous_duel_score = fencer->GetUIntData (match, "Quest::DuelScore");
        Attribute *duel_score_attr     = fencer->GetAttribute (&duel_score_attr_id);

        if (duel_score_attr)
        {
          fencer->SetAttributeValue (&duel_score_attr_id,
                                     duel_score_attr->GetUIntValue () - previous_duel_score);
          fencer->RemoveData (match, "Quest::DuelScore");
        }
      }
    }
  }
}
