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

#include <math.h>

#include "util/player.hpp"
#include "util/attribute.hpp"
#include "../../match.hpp"

#include "elo.hpp"

namespace Swiss
{
  // --------------------------------------------------------------------------------
  Elo::Elo ()
    : Object ("Swiss::Elo")
  {
    _fencers = nullptr;
  }

  // --------------------------------------------------------------------------------
  Elo::~Elo ()
  {
    CancelBatch ();
  }

  // --------------------------------------------------------------------------------
  void Elo::ProcessBatch (GList *matches)
  {
    for (GList *m = matches; m; m = g_list_next (m))
    {
      Match *match = (Match *) m->data;

      PreserveInitialValue (match);
      Evaluate             (match);
    }
  }

  // --------------------------------------------------------------------------------
  void Elo::PreserveInitialValue (Match *match)
  {
    Player::AttributeId elo_attr_id ("elo");

    for (guint i = 0; i < 2; i++)
    {
      Player *fencer = match->GetOpponent (i);

      if (g_list_find (_fencers,
                       fencer) == nullptr)
      {
        Attribute *elo_attr = fencer->GetAttribute (&elo_attr_id);

        if (elo_attr == nullptr)
        {
          fencer->SetAttributeValue (&elo_attr_id,
                                     1000);
          elo_attr = fencer->GetAttribute (&elo_attr_id);
        }

        fencer->SetData (this,
                         "Elo::recovery",
                         GUINT_TO_POINTER (elo_attr->GetUIntValue ()));

        _fencers = g_list_prepend (_fencers,
                                   fencer);
      }
    }

  }

  // --------------------------------------------------------------------------------
  void Elo::Evaluate (Match *match)
  {
    Player::AttributeId elo_attr_id ("elo");
    gdouble             elo[2];
    gdouble             probability[2];

    // elo
    for (guint f = 0; f < 2; f++)
    {
      Player    *fencer   = (Player *) match->GetOpponent (f);
      Attribute *elo_attr = fencer->GetAttribute (&elo_attr_id);

      elo[f] = elo_attr->GetUIntValue ();
    }

    // probability
    probability[0] = 1 / (1 + pow (10, (elo[1] - elo[0]) / 400));
    probability[1] = 1 - probability[0];

    for (guint f = 0; f < 2; f++)
    {
      Player  *fencer = (Player *) match->GetOpponent (f);
      gdouble  new_elo;

      if (fencer == match->GetWinner ())
      {
        new_elo = round (elo[f] + K*(1-probability[f]));
      }
      else
      {
        new_elo = round (elo[f] + K*(-probability[f]));
      }

      fencer->SetAttributeValue (&elo_attr_id,
                                 (guint) new_elo);
    }
  }

  // --------------------------------------------------------------------------------
  void Elo::CancelBatch ()
  {
    Player::AttributeId elo_attr_id ("elo");

    for (GList *f = _fencers; f; f = g_list_next (f))
    {
      Player *fencer   = (Player *) f->data;
      guint   recovery = fencer->GetUIntData (this,
                                              "Elo::recovery");

      fencer->SetAttributeValue (&elo_attr_id,
                                 recovery);

      fencer->RemoveData (this,
                          "Elo::recovery");
    }

    g_list_free (_fencers);
    _fencers = nullptr;
  }
}
