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

namespace Quest
{
  // --------------------------------------------------------------------------------
  Elo::Elo ()
    : Object ("Quest::Elo"),
      Generic::Elo (32)
  {
    _table_pattern = g_regex_new ("[0-9]+-[0-9]+\\.[0-9]+",
                                  G_REGEX_OPTIMIZE,
                                  (GRegexMatchFlags) 0,
                                  nullptr);

  }

  // --------------------------------------------------------------------------------
  Elo::~Elo ()
  {
    g_regex_unref (_table_pattern);
  }

  // --------------------------------------------------------------------------------
  void Elo::ProcessBatch (GList *matches)
  {
    Player::AttributeId elo_attr_id ("elo");

    Generic::Elo::ProcessBatch (matches);

    for (GList *m = matches; m; m = g_list_next (m))
    {
      Match *match = (Match *) m->data;

      const gchar *name         = match->GetName (); ;
      GMatchInfo  *pattern_info;

      if (g_regex_match (_table_pattern,
                         name,
                         (GRegexMatchFlags) 0,
                         &pattern_info))
      {
        guint   bonus  = 10;
        Player *looser = match->GetLooser ();
        Player *winner = nullptr;

        if (g_strcmp0 (name, "1-2.1") == 0)
        {
          winner = match->GetWinner ();
          bonus = 120;
        }
        else if (g_strcmp0 (name, "3-2.1") == 0)
        {
          winner = match->GetWinner ();
          bonus = 30;
        }

        if (winner)
        {
          Attribute *elo_attr = winner->GetAttribute (&elo_attr_id);
          guint      elo      = elo_attr->GetUIntValue ();

          elo += bonus*2;
          winner->SetAttributeValue (&elo_attr_id,
                                     (guint) elo);
        }
        if (looser)
        {
          Attribute *elo_attr = looser->GetAttribute (&elo_attr_id);
          guint      elo      = elo_attr->GetUIntValue ();

          elo += bonus;
          looser->SetAttributeValue (&elo_attr_id,
                                     (guint) elo);
        }
      }

      g_match_info_free (pattern_info);
    }
  }

  // --------------------------------------------------------------------------------
  guint Elo::GetBonus (Match *match)
  {
    return 0;
  }
}
