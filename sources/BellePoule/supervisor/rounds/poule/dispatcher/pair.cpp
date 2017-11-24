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

#include "opponent.hpp"
#include "pair.hpp"

namespace Pool
{
  // --------------------------------------------------------------------------------
  Pair::Pair (guint     iteration,
              Opponent *a,
              Opponent *b)
    : Object ("Pair")
  {
    _a = a;
    _b = b;

    _a->SetFitness (iteration);
    _b->SetFitness (iteration);

    _a->Use (b);
  }

  // --------------------------------------------------------------------------------
  Pair::~Pair ()
  {
  }

  // --------------------------------------------------------------------------------
  void Pair::ResetFitness ()
  {
    _a_fitness = -1;
    _b_fitness = -1;
  }

  // --------------------------------------------------------------------------------
  void Pair::SetFitness (Pair  *tested_pair,
                         guint  fitness)
  {
    if (_a_fitness == -1)
    {
      if (tested_pair->HasOpponent (_a))
      {
        _a_fitness = fitness;
        _a->RefreshFitnessProfile (fitness);
      }
    }

    if (_b_fitness == -1)
    {
      if (tested_pair->HasOpponent (_b))
      {
        _b_fitness = fitness;
        _b->RefreshFitnessProfile (fitness);
      }
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Pair::HasFitnessError ()
  {
    if ((_a_fitness == 0) || (_b_fitness == 0))
    {
      g_print (RED "%s - %s\n" ESC, _a->GetName (), _b->GetName ());
    }
    return ((_a_fitness == 0) || (_b_fitness == 0));
  }

  // --------------------------------------------------------------------------------
  guint Pair::GetAFitness ()
  {
    if (_a_fitness == -1)
    {
      return 0;
    }
    return _a_fitness;
  }

  // --------------------------------------------------------------------------------
  guint Pair::GetBFitness ()
  {
    if (_b_fitness == -1)
    {
      return 0;
    }
    return _b_fitness;
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

  // --------------------------------------------------------------------------------
  guint Pair::GetA ()
  {
    if (_a)
    {
      return _a->GetId ();
    }

    return 0;
  }
  // --------------------------------------------------------------------------------
  guint Pair::GetB ()
  {
    if (_b)
    {
      return _b->GetId ();
    }

    return 0;
  }

  // --------------------------------------------------------------------------------
  void Pair::Dump ()
  {
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
}
