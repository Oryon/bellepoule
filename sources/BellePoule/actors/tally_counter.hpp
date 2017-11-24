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

#pragma once

#include "util/object.hpp"

namespace People
{
  class TallyCounter : public Object
  {
    public:
      TallyCounter ();

      void Monitor (Player *player);

      void Drop (Player *player);

      void OnAttendingChanged (Player   *player,
                               gboolean  present);

      guint GetTotalFencerCount ();

      guint GetTotalCount ();

      guint GetPresentsCount ();

      guint GetAbsentsCount ();

      void SetTeamMode ();

      void DisableTeamMode ();

    private:
      struct Mode
      {
        guint _total_count;
        guint _present_count;
      };

      Mode     _normal_mode;
      Mode     _team_mode;
      gboolean _team_mode_enabled;

      ~TallyCounter ();

      Mode *GetMode (Player *player);
  };
}
