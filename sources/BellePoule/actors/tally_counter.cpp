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

#include "util/attribute.hpp"
#include "util/player.hpp"

#include "tally_counter.hpp"

namespace People
{
  // --------------------------------------------------------------------------------
  TallyCounter::TallyCounter ()
    : Object ("TallyCounter")
  {
    _normal_mode._total_count   = 0;
    _normal_mode._present_count = 0;

    _team_mode._total_count   = 0;
    _team_mode._present_count = 0;

    _team_mode_enabled  = FALSE;
  }

  // --------------------------------------------------------------------------------
  TallyCounter::~TallyCounter ()
  {
  }

  // --------------------------------------------------------------------------------
  void TallyCounter::Monitor (Player *player)
  {
    Player::AttributeId  attr_id ("attending");
    Attribute           *attr = player->GetAttribute (&attr_id);
    Mode                *mode = GetMode (player);

    mode->_total_count++;
    if (attr && (attr->GetUIntValue () == TRUE))
    {
      mode->_present_count++;
    }
  }

  // --------------------------------------------------------------------------------
  void TallyCounter::Drop (Player *player)
  {
    Player::AttributeId  attr_id ("attending");
    Attribute           *attr = player->GetAttribute (&attr_id);
    Mode                *mode = GetMode (player);

    mode->_total_count--;
    if (attr && (attr->GetUIntValue () == TRUE))
    {
      mode->_present_count--;
    }
  }

  // --------------------------------------------------------------------------------
  void TallyCounter::OnAttendingChanged (Player   *player,
                                         gboolean  present)
  {
    Mode *mode = GetMode (player);

    if (present)
    {
      mode->_present_count++;
    }
    else
    {
      mode->_present_count--;
    }
  }

  // --------------------------------------------------------------------------------
  guint TallyCounter::GetTotalFencerCount ()
  {
    return _normal_mode._total_count;
  }

  // --------------------------------------------------------------------------------
  guint TallyCounter::GetTotalCount ()
  {
    return GetPresentsCount () + GetAbsentsCount ();
  }

  // --------------------------------------------------------------------------------
  guint TallyCounter::GetPresentsCount ()
  {
    if (_team_mode_enabled)
    {
      return _team_mode._present_count;
    }
    else
    {
      return _normal_mode._present_count;
    }
  }

  // --------------------------------------------------------------------------------
  guint TallyCounter::GetAbsentsCount ()
  {
    if (_team_mode_enabled)
    {
      return _team_mode._total_count - _team_mode._present_count - 1;
    }
    else
    {
      return _normal_mode._total_count - _normal_mode._present_count;
    }
  }

  // --------------------------------------------------------------------------------
  void TallyCounter::SetTeamMode ()
  {
    _team_mode_enabled = TRUE;
  }

  // --------------------------------------------------------------------------------
  void TallyCounter::DisableTeamMode ()
  {
    _team_mode_enabled = FALSE;
  }

  // --------------------------------------------------------------------------------
  TallyCounter::Mode *TallyCounter::GetMode (Player *player)
  {
    if (player->Is ("Team"))
    {
      return &_team_mode;
    }
    else
    {
      return &_normal_mode;
    }
  }
}
