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

#include "object.hpp"

class Score_c : public Object_c
{
  public:
    Score_c  (guint max_score);
    ~Score_c ();

    gboolean IsKnown ();

    guint Get ();

    gchar *GetImage ();

    void Set (guint score);

    void Clean ();

    gboolean IsValid ();

    gboolean IsConsistentWith (Score_c *with);

  private:
    gboolean  _is_known;
    guint     _max_score;
    guint     _score;
};

#endif
