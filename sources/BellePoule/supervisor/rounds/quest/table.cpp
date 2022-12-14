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

#include "util/data.hpp"
#include "util/player.hpp"
#include "duel_score.hpp"
#include "table.hpp"
#include "point_system.hpp"

namespace Quest
{
  const gchar *BonusTable::_class_name     = N_ ("Table");
  const gchar *BonusTable::_xml_class_name = "TableauSuisse";

  // --------------------------------------------------------------------------------
  BonusTable::BonusTable (StageClass *stage_class)
    : Object ("Quest::BonusTable"),
      Table::Supervisor (stage_class),
      _spread_randomly(false)
  {
    _max_score->Release ();
    _max_score = new Data ("ScoreMax",
                           15);

    _fenced_places->Release ();
    _fenced_places = new Data ("PlacesTirees",
                               Table::Supervisor::THIRD_PLACES);
  }

  // --------------------------------------------------------------------------------
  BonusTable::~BonusTable ()
  {
  }

  // --------------------------------------------------------------------------------
  Generic::PointSystem *BonusTable::GetPointSystem (Table::TableSet *table_set)
  {
    return new PointSystem (this);
  }

  // --------------------------------------------------------------------------------
  gboolean BonusTable::ScoreOverflowAllowed ()
  {
    return TRUE;
  }

  // --------------------------------------------------------------------------------
  GSList *BonusTable::SpreadAttendees (GSList *attendees)
  {
    if (_spread_randomly) {
      // This is not really used, but could be some day, so left it as an option
      return g_slist_sort_with_data (attendees,
                                       (GCompareDataFunc) Player::RandomCompare,
                                       GINT_TO_POINTER (GetAntiCheatToken ()));
    } else {
      // Default approach is to sort players by stage rank
      Player::AttributeId attr_id = Player::AttributeId ("stage_start_rank", this);
      return g_slist_sort_with_data (attendees,
                                     (GCompareDataFunc) Player::Compare,
                                     &attr_id);
    }
  }

  // --------------------------------------------------------------------------------
  const gchar *BonusTable::GetXmlClassName ()
  {
    return _xml_class_name;
  }

  // --------------------------------------------------------------------------------
  void BonusTable::Declare ()
  {
    RegisterStageClass (gettext (_class_name),
                        _xml_class_name,
                        CreateInstance,
                        EDITABLE|REMOVABLE);
  }

  // --------------------------------------------------------------------------------
  Stage *BonusTable::CreateInstance (StageClass *stage_class)
  {
    return new BonusTable (stage_class);
  }
}
