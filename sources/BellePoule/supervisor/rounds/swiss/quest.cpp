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

#include "quest.hpp"

namespace Swiss
{
  // --------------------------------------------------------------------------------
  Quest::Quest (Object *owner)
    : Object ("Swiss::Quest")
  {
    _owner = owner;
  }

  // --------------------------------------------------------------------------------
  Quest::~Quest ()
  {
  }

  // --------------------------------------------------------------------------------
  void Quest::EvaluateMatch (Match *match)
  {
    Player::AttributeId quest_attr_id ("score_quest", _owner);

    CancelMatch (match);

    if (match->IsDropped ())
    {
      for (guint i = 0; i < 2; i++)
      {
        Player    *fencer     = match->GetOpponent (i);
        Attribute *quest_attr = fencer->GetAttribute (&quest_attr_id);

        if (match->GetScore (fencer)->IsOut () == FALSE)
        {
          guint new_quest = 2;

          fencer->SetData (match,
                           "Round::quest",
                           GUINT_TO_POINTER (2));

          if (quest_attr)
          {
            new_quest += quest_attr->GetUIntValue ();
          }
          fencer->SetAttributeValue (&quest_attr_id,
                                     new_quest);
        }
      }
    }
    else if (match->IsOver ())
    {
      Player *winner = match->GetWinner ();
      Player *looser = match->GetLooser ();

      if (winner && looser)
      {
        guint      winner_score = match->GetScore (winner)->Get ();
        guint      looser_score = match->GetScore (looser)->Get ();
        guint      delta        = winner_score - looser_score;
        Attribute *quest_attr   = winner->GetAttribute (&quest_attr_id);
        guint      new_quest;

        if (delta < 4)
        {
          new_quest = 1;
        }
        else if (4 <= delta && delta <= 7)
        {
          new_quest = 2;
        }
        else if (8 <= delta && delta <= 11)
        {
          new_quest = 3;
        }
        else
        {
          new_quest = 4;
        }

        winner->SetData (match,
                         "Round::quest",
                         GUINT_TO_POINTER (new_quest));

        if (quest_attr)
        {
          new_quest += quest_attr->GetUIntValue ();
        }
        winner->SetAttributeValue (&quest_attr_id,
                                   new_quest);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Quest::CancelMatch (Match *match)
  {
    Player::AttributeId quest_attr_id ("score_quest", _owner);

    for (guint i = 0; i < 2; i++)
    {
      Player *fencer = match->GetOpponent (i);

      if (fencer)
      {
        guint      previous_quest = fencer->GetUIntData (match, "Round::quest");
        Attribute *quest_attr     = fencer->GetAttribute (&quest_attr_id);

        if (quest_attr)
        {
          fencer->SetAttributeValue (&quest_attr_id,
                                     quest_attr->GetUIntValue () - previous_quest);
          fencer->RemoveData (match, "Round::quest");
        }
      }
    }
  }
}
