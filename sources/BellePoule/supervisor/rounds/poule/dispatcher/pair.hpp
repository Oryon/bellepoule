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

#include <glib.h>

#include "util/object.hpp"

namespace Pool
{
  class Opponent;

  class Pair : public Object
  {
    public:
      Pair (guint     iteration,
            Opponent *a,
            Opponent *b);

      void ResetFitness ();

      void SetFitness (Pair  *tested_pair,
                       guint  fitness);

      gboolean HasFitnessError ();

      guint GetA ();

      guint GetB ();

      guint GetAFitness ();

      guint GetBFitness ();

      void Dump () override;

      gboolean HasOpponent (Opponent *o);

    private:
      Opponent *_a;
      Opponent *_b;
      gint      _a_fitness;
      gint      _b_fitness;

      ~Pair () override;
  };
}
