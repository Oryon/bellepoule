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

#ifndef opponent_hpp
#define opponent_hpp

#include <glib.h>

#include "util/object.hpp"

namespace Pool
{
  class Pair;

  class Opponent : public Object
  {
    public:
      Opponent (guint id);

      guint GetId ();

      void Feed (GList *opponent_list);

      Opponent *GetBestOpponent ();

      void SetFitness (guint fitness);

      void Dump ();

      static gint CompareFitness (Opponent *a,
                                  Opponent *b);

    private:
      guint  _id;
      guint  _fitness;
      GList *_opponent_list;

      virtual ~Opponent ();
  };
}

#endif
