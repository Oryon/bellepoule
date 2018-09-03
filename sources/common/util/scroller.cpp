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

#include "scroller.hpp"

const gdouble Scroller::HOT_AREA = 50.0;

// --------------------------------------------------------------------------------
Scroller::Scroller (GtkScrolledWindow *scrolled_window)
  : Object ("Scroller")
{
  _metronome = 0;

  _vadj = gtk_scrolled_window_get_vadjustment (scrolled_window);
}

// --------------------------------------------------------------------------------
Scroller::~Scroller ()
{
  Deactivate ();
}

// --------------------------------------------------------------------------------
void Scroller::Activate ()
{
  _lower_stopper = gtk_adjustment_get_lower (_vadj);
  _page_h        = gtk_adjustment_get_page_size (_vadj);
  _upper_stopper = gtk_adjustment_get_upper (_vadj) - _page_h;
}

// --------------------------------------------------------------------------------
void Scroller::Deactivate ()
{
  if (_metronome)
  {
    g_source_remove (_metronome);
    _metronome = 0;
  }
}

// --------------------------------------------------------------------------------
gboolean Scroller::Scroll (Scroller *scroller)
{
  gdouble adjy = gtk_adjustment_get_value (scroller->_vadj);

  printf ("%f\n", scroller->_step);
  gtk_adjustment_set_value (scroller->_vadj,
                            adjy + scroller->_step);

  return G_SOURCE_CONTINUE;
}

// --------------------------------------------------------------------------------
void Scroller::OnNewCursorPosition (gdouble x,
                                    gdouble y)
{
  gdouble adjy      = gtk_adjustment_get_value (_vadj);
  gdouble y_in_page = y - adjy;

  if ((y_in_page < HOT_AREA) && (adjy > _lower_stopper))
  {
    _step = -GetScrollStep (HOT_AREA - y_in_page);
  }
  else if ((y_in_page > _page_h - HOT_AREA) && (adjy < _upper_stopper))
  {
    _step = GetScrollStep (y_in_page - (_page_h - HOT_AREA));
  }
  else
  {
    _step = 0.0;
  }

  if (_metronome == 0)
  {
    Scroll (this);
    _metronome = g_timeout_add (5,
                                (GSourceFunc) Scroll,
                                this);
  }
}

// --------------------------------------------------------------------------------
gdouble Scroller::GetScrollStep (gdouble dist)
{
  gdouble step;

  if (dist > HOT_AREA)
  {
    dist = HOT_AREA;
  }

  step = (dist / (HOT_AREA/SPEED_LEVEL)) * 2;
  if (step <= 0)
  {
    step = 1;
  }

  return step;
}
