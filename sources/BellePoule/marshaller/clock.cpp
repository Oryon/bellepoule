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

#include "network/message.hpp"

#include "clock.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  Clock::Clock (Listener *listener)
    : Object ("Clock")
  {
    _listener = listener;
    _tag      = 0;
    _offset   = 0;
    _absolute = NULL;

    OnTimeout (this);
  }

  // --------------------------------------------------------------------------------
  Clock::~Clock ()
  {
    if (_tag)
    {
      g_source_remove (_tag);
    }

    if (_absolute)
    {
      g_date_time_unref (_absolute);
    }
  }

  // --------------------------------------------------------------------------------
  void Clock::Set (GDateTime *to)
  {
    _offset = g_date_time_difference (to,
                                      _absolute);
    OnTimeout (this);

    Disclose ("ClockOffset");
    Spread ();
  }

  // --------------------------------------------------------------------------------
  GDateTime *Clock::RetreiveNow ()
  {
    GDateTime *now      = g_date_time_new_now_local ();
    GDateTime *adjusted = g_date_time_add (now, _offset);

    g_date_time_unref (now);

    return adjusted;
  }

  // --------------------------------------------------------------------------------
  void Clock::FeedParcel (Net::Message *parcel)
  {
    parcel->Set ("offset", (gint) (_offset/G_TIME_SPAN_SECOND));
  }

  // --------------------------------------------------------------------------------
  void Clock::SetupTimeout ()
  {
    GDateTime *now     = g_date_time_new_now_local ();
    guint      seconds = g_date_time_get_second (now);

    _tag = g_timeout_add_seconds (60-seconds,
                                  (GSourceFunc) OnTimeout,
                                  this);
    g_date_time_unref (now);
  }

  // --------------------------------------------------------------------------------
  gboolean Clock::OnTimeout (Clock *clock)
  {
    if (clock->_absolute)
    {
      g_date_time_unref (clock->_absolute);
    }
    clock->_absolute = g_date_time_new_now_local ();

    {
      GDateTime *adjusted = g_date_time_add (clock->_absolute, clock->_offset);
      gchar     *time     = g_strdup_printf ("%02d:%02d",
                                             g_date_time_get_hour   (adjusted),
                                             g_date_time_get_minute (adjusted));

      clock->_listener->OnNewTime (time);
      g_free (time);
      g_date_time_unref (adjusted);
    }

    clock->SetupTimeout ();

    return G_SOURCE_REMOVE;
  }
}
