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

#include "pair.hpp"

namespace Pool
{
  // --------------------------------------------------------------------------------
  Pair::Pair (guint     iteration,
              Opponent *a,
              Opponent *b)
  {
    _a = a;
    _b = b;

    _a->SetFitness (iteration);
    _b->SetFitness (iteration);
  }

  // --------------------------------------------------------------------------------
  Pair::~Pair ()
  {
  }

  // --------------------------------------------------------------------------------
  void Pair::Dump ()
  {
#if 0
    if (_fitness == 0)
    {
      printf (RED);
    }

    printf ("%2d(%d) - %2d(%d)  : %2d\n" ESC,
            _a->GetId (), _a_fitness,
            _b->GetId (), _b_fitness,
            _fitness);
#endif

    printf ("%2d(", _a->GetId ());
    if (_a_fitness == -1)
    {
      printf (" )");
    }
    else
    {
      if (_a_fitness == 0) printf (RED);
      printf ("%d" ESC ")", _a_fitness);
    }

    printf (" - %2d(", _b->GetId ());
    if (_b_fitness == -1)
    {
      printf (" )\n");
    }
    else
    {
      if (_b_fitness == 0) printf (RED);
      printf ("%d" ESC ")\n", _b_fitness);
    }
  }

  // --------------------------------------------------------------------------------
  void Pair::SetFitness (gint fitness)
  {
    _fitness = fitness;
  }

  // --------------------------------------------------------------------------------
  gint Pair::GetFitness ()
  {
    return MIN (_a_fitness, _b_fitness);
  }

  // --------------------------------------------------------------------------------
  gboolean Pair::HasOpponent (Opponent *o)
  {
    if (o)
    {
      return (   (_a == o)
              || (_b == o));
    }

    return FALSE;
  }
}
