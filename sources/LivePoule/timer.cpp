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

#include "timer.hpp"

// --------------------------------------------------------------------------------
Timer::Timer (GtkWidget *w)
  : Object ("Timer")
{
  _timeout_id = 0;
  _label      = GTK_LABEL (w);
  _start_time = 0;
  Set (0);
}

// --------------------------------------------------------------------------------
Timer::~Timer ()
{
  if (_timeout_id > 0)
  {
    g_source_remove (_timeout_id);
  }
}

// --------------------------------------------------------------------------------
void Timer::Set (gint sec)
{
  if (sec)
  {
    _remaining = sec * 1000000;
    Refresh ();
  }
}

// --------------------------------------------------------------------------------
void Timer::Refresh ()
{
  gint   sec  = _remaining / 1000000;
  gchar *text = g_strdup_printf ("%02d:%02d", sec/60, sec%60);

  gtk_label_set_text (_label,
                      text);
  g_free (text);
}

// --------------------------------------------------------------------------------
void Timer::SetState (const gchar *state)
{
  if (state)
  {
    if (strcmp (state, "RUNNING") == 0)
    {
      _start_time         = g_get_real_time ();
      _remaining_at_start = _remaining;

      _timeout_id = g_timeout_add_seconds (1,
                                           (GSourceFunc) OnTimeout,
                                           this);
    }
    else if (_timeout_id > 0)
    {
      g_source_remove (_timeout_id);
      _timeout_id = 0;
    }
  }
}

// --------------------------------------------------------------------------------
gboolean Timer::OnTimeout (Timer *timer)
{
  gint64 current_time = g_get_real_time ();

  timer->_remaining = timer->_remaining_at_start - (current_time - timer->_start_time);
  if (timer->_remaining < 0)
  {
    timer->_remaining = 0;
  }

  timer->Refresh ();

  return TRUE;
}
