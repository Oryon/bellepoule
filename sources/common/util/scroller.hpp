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

class Scroller : public Object
{
  public:
    Scroller (GtkScrolledWindow *scrolled_window);

    void OnNewCursorPosition (gdouble x,
                              gdouble y);

    void Activate ();

    void Deactivate ();

  private:
    static const guint   SPEED_LEVEL = 4;
    static const gdouble HOT_AREA;

    GtkAdjustment *_vadj;
    gdouble        _lower_stopper;
    gdouble        _upper_stopper;
    gdouble        _page_h;
    guint          _metronome;
    gdouble        _step;

    ~Scroller ();

    gdouble GetScrollStep (gdouble dist);

    static gboolean Scroll (Scroller *scroller);

};
