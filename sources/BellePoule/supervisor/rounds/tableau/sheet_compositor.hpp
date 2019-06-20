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

#include "util/object.hpp"

class Match;

namespace Table
{
  class Page : public Object
  {
    public:
      Page ();

      GList *FillWith (GList *what);

      gboolean Append (GList *what);

      void StartIterator ();

      Match *GetNextMatch ();

    private:
      guint  _accepted_size;
      GList *_matches;
      GList *_current_match;

      ~Page ();
  };

  class SheetCompositor : public Object
  {
    public:
      SheetCompositor ();

      void Reset ();

      void AddMatch (Match *match);

      guint GetPageCount (gdouble paper_w,
                          gdouble paper_h);

      guint MatchPerSheet ();

      Page *GetPage (guint p);

    private:
      GList *_match_by_referee;
      GList *_pages;
      guint  _nb_match_per_sheet;

      virtual ~SheetCompositor ();

      static gint CompareGListLength (GList *a,
                                      GList *b);

      Page *CreatePage ();
  };
}
