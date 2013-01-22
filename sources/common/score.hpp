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

#ifndef score_hpp
#define score_hpp

#include "util/object.hpp"
#include "util/data.hpp"

class Score : public Object
{
  public:
    Score  (Data *max);
    virtual ~Score ();

    gboolean IsKnown ();

    guint Get ();

    gchar *GetImage ();

    void Set (gint     score,
              gboolean is_the_best);

    void Clean ();

    void Drop ();

    void Restore ();

    gboolean IsValid ();

    gboolean IsTheBest ();

    gboolean IsConsistentWith (Score *with);

  private:
    gboolean   _is_known;
    gboolean   _is_dropped;
    Data      *_max;
    guint      _score;
    gboolean   _is_the_best;
};

#endif
