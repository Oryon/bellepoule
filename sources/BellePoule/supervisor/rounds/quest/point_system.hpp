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

#include <util/object.hpp>
#include "../common/point_system.hpp"

namespace Quest
{
  class DuelScore;

  class PointSystem : public Generic::PointSystem
  {
    public:
      PointSystem (Stage *stage);

    private:
      DuelScore *_duel_score;

      virtual ~PointSystem ();

      void RateMatch (Match *match) override;

      virtual void Reset () override;

      gint Compare (Player *A,
                    Player *B) override;
  };
}
