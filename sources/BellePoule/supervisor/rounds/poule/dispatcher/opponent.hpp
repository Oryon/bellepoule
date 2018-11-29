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

class Fencer;

namespace Pool
{
  class Opponent : public Object
  {
    public:
      Opponent (guint   id,
                Fencer *fencer);

      void SetQuark (GQuark quark);

      guint GetId ();

      void Feed (GList *opponent_list);

      Opponent *GetBestOpponent ();

      void SetFitness (guint fitness);

      gint GetFitness ();

      void RefreshFitnessProfile (guint fitness);

      void Lock (GQuark teammate_quark,
                 guint  teammate_count);

      gboolean IsCompatibleWith (Opponent *with);

      void Use (Opponent *who);

      gchar *GetName ();

      void Dump (guint max_fitness);

      static gint CompareFitness (Opponent *a,
                                  Opponent *b);

      static void ResetFitnessProfile (Opponent *o,
                                       guint     size);

    private:
      guint   _id;
      Fencer *_fencer;
      GQuark  _quark;
      guint   _lock;
      gint    _fitness;
      guint  *_fitness_profile;
      guint   _max_fitness;
      GList  *_opponent_list;

      ~Opponent () override;
  };
}
