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
#include "../../bonus.hpp"

class Match;

namespace Swiss
{
  class Quest;
  class Elo;

  class Bonus : public Generic::Bonus
  {
    public:
      Bonus (Object *owner);

      virtual void AuditMatch (Match *match) override;

      virtual void SumUp () override;

    protected:
      GList *_matches;
      Quest *_quest;
      Elo   *_elo;

      virtual ~Bonus ();
  };
}
