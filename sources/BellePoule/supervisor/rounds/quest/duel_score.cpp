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
#include "match_figure.hpp"

#include "duel_score.hpp"

namespace Quest
{
  // --------------------------------------------------------------------------------
  DuelScore::DuelScore (Object *owner)
    : Object ("Quest::DuelScore")
  {
    _score_figure = new MatchFigure ("score_quest",
                                     owner);
    _nb_matchs_figure = new MatchFigure ("nb_matchs",
                                         owner);
    _victory_figure = new MatchFigure ("victories_count",
                                       owner);
  }

  // --------------------------------------------------------------------------------
  DuelScore::~DuelScore ()
  {
    _score_figure->Release     ();
    _nb_matchs_figure->Release ();
    _victory_figure->Release   ();
  }

  // --------------------------------------------------------------------------------
  void DuelScore::RateMatch (Match *match)
  {
    CancelMatch (match);

    if (match->IsDropped ())
    {
      for (guint i = 0; i < 2; i++)
      {
        Player *fencer = match->GetOpponent (i);

        if (match->GetScore (fencer)->IsOut () == FALSE)
        {
          _score_figure->AddToFencer (fencer,
                                      match,
                                      2);
          _victory_figure->AddToFencer (fencer,
                                        match,
                                        1);
        }
        _nb_matchs_figure->AddToFencer (fencer,
                                        match,
                                        1);
      }
    }
    else if (match->IsOver ())
    {
      Player *winner = match->GetWinner ();
      Player *looser = match->GetLooser ();

      if (winner && looser)
      {
        {
          guint winner_score   = match->GetScore (winner)->Get ();
          guint looser_score   = match->GetScore (looser)->Get ();
          guint delta          = winner_score - looser_score;
          guint new_duel_score;

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

          _score_figure->AddToFencer (winner,
                                      match,
                                      new_duel_score);
        }

        _victory_figure->AddToFencer (winner,
                                      match,
                                      1);

        _nb_matchs_figure->AddToFencer (winner,
                                        match,
                                        1);
        _nb_matchs_figure->AddToFencer (looser,
                                        match,
                                        1);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void DuelScore::CancelMatch (Match *match)
  {
    for (guint i = 0; i < 2; i++)
    {
      Player *fencer = match->GetOpponent (i);

      if (fencer)
      {
        _score_figure->RemoveFromFencer (fencer,
                                         match);

        _nb_matchs_figure->RemoveFromFencer (fencer,
                                             match);

        _victory_figure->RemoveFromFencer (fencer,
                                           match);
      }
    }
  }
}
