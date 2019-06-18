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

#include <stdlib.h>

#include "attendees.hpp"

// --------------------------------------------------------------------------------
Attendees::Attendees (Listener *listener,
                      Module   *owner)
  : Object ("Attendees")
{
  _listener = listener;
  _owner    = owner;
}

// --------------------------------------------------------------------------------
Attendees::~Attendees ()
{
  g_slist_free (_absents);
  g_slist_free (_presents);
}

// --------------------------------------------------------------------------------
Module *Attendees::GetOwner ()
{
  return _owner;
}

// --------------------------------------------------------------------------------
void Attendees::SetPresents (GSList *presents)
{
  g_slist_free (_presents);
  _presents = presents;
}

// --------------------------------------------------------------------------------
void Attendees::SetAbsents (GSList *absents)
{
  g_slist_free (_absents);
  _absents = absents;
}

// --------------------------------------------------------------------------------
GSList *Attendees::GetPresents ()
{
  return _presents;
}

// --------------------------------------------------------------------------------
GSList *Attendees::GetAbsents ()
{
  return _absents;
}

// --------------------------------------------------------------------------------
void Attendees::Toggle (Player *fencer)
{
  _listener->OnAttendeeToggled (fencer);
}
