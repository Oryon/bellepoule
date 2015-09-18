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
Attendees::Attendees ()
{
  _global_list = NULL;
  _shortlist   = NULL;
}

// --------------------------------------------------------------------------------
Attendees::Attendees  (Attendees *from,
                       GSList    *shortlist)
: Object ("Attendees")
{
  if (from)
  {
    _global_list = from->_global_list;
  }
  else
  {
    _global_list = NULL;
  }
  _shortlist = shortlist;
}

// --------------------------------------------------------------------------------
Attendees::~Attendees ()
{
}

// --------------------------------------------------------------------------------
void Attendees::SetGlobalList (GSList *global_list)
{
  _global_list = global_list;
}

// --------------------------------------------------------------------------------
GSList *Attendees::GetGlobalList ()
{
  return _global_list;
}

// --------------------------------------------------------------------------------
GSList *Attendees::GetShortList ()
{
  return _shortlist;
}
