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

#include "clock.hpp"

// --------------------------------------------------------------------------------
Clock::Clock ()
  : Object ("Clock")
{
  g_get_current_time (&_last_timeval);
}

// --------------------------------------------------------------------------------
Clock::~Clock ()
{
}

// --------------------------------------------------------------------------------
void Clock::PrintElapsed (const gchar *tag)
{
  GTimeVal current_timeval;
  GTimeVal diff_timeval;

  g_get_current_time (&current_timeval);

  GetTimevalDiff (&diff_timeval,
                  &current_timeval,
                  &_last_timeval);

  printf (BLUE "%s " ESC "%ld (%ld/%ld)\n",
          tag,
          diff_timeval.tv_sec*G_USEC_PER_SEC + diff_timeval.tv_usec,
          diff_timeval.tv_sec,
          diff_timeval.tv_usec);

  _last_timeval = current_timeval;
}

// --------------------------------------------------------------------------------
void Clock::GetTimevalDiff (GTimeVal *result,
                            GTimeVal *x,
                            GTimeVal *y)
{
  if (x->tv_usec < y->tv_usec)
  {
    gint nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;

    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000)
  {
    gint nsec = (x->tv_usec - y->tv_usec) / 1000000;

    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  result->tv_sec  = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;
}
