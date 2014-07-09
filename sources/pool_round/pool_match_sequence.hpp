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

#ifndef pool_match_sequence_hpp
#define pool_match_sequence_hpp

#include <glib.h>

#include "object.hpp"

namespace Pool
{
  class Pair;

  class Opponent : public Object
  {
    public:
      guint _id;

      Opponent (guint id);

      void Feed (GList *opponent_list);

      Opponent *GetBestOpponent ();

      void AddPair (Pair  *pair,
                    guint  fitness);

      guint GetLastMatch ();

      void Dump ();

      static gint CompareFitness (Opponent *a,
                                  Opponent *b);

    private:
      guint  _fitness;
      GList *_opponent_list;
      GList *_pair_list;

      virtual ~Opponent ();
  };

  class Pair : public Object
  {
    public:
      Opponent *_a;
      Opponent *_b;

      Pair (guint     iteration,
            Opponent *a,
            Opponent *b);

      void SetFitness (guint fitness);

      guint GetFitness ();

      void Dump ();

      gboolean HasSameOpponent (Pair *than);

      static gint CompareFitness (GList *a,
                                  GList *b);

    private:
      guint _fitness;

      virtual ~Pair ();
  };

  class MatchSequence : public Object
  {
    public:
      MatchSequence (guint pool_size);

    private:
      GList *_pair_list;

      virtual ~MatchSequence ();

      GList *GetOpponentList (guint pool_size);

      void SpreadOpponents (GList *opponent_list);

      void CreatePairs (GList *opponent_list);

      void FixErrors ();

      guint GetFitness (Pair *pair);

      void RefreshFitness ();

      void Dump ();
  };
}

#endif
