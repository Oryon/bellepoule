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

#include "pool_allocator.hpp"
#include "pool_supervisor.hpp"

#include "stage.hpp"

// --------------------------------------------------------------------------------
Stage_c::Stage_c (gchar *name)
{
  _name   = name;
  _locked = false;
  _result = NULL;
}

// --------------------------------------------------------------------------------
Stage_c::~Stage_c ()
{
  FreeResult ();
}

// --------------------------------------------------------------------------------
gchar *Stage_c::GetName ()
{
  return _name;
}

// --------------------------------------------------------------------------------
void Stage_c::FreeResult ()
{
  for (guint i = 0; i <  g_slist_length (_result); i++)
  {
    Player_c *p;

    p = (Player_c *) g_slist_nth_data (_result, i);
    p->Release ();
  }

  if (_result)
  {
    g_slist_free (_result);
    _result = NULL;
  }
}

// --------------------------------------------------------------------------------
void Stage_c::Lock ()
{
  _locked = true;
  FreeResult ();
  OnLocked ();
}

// --------------------------------------------------------------------------------
void Stage_c::UnLock ()
{
  _locked = false;
  OnUnLocked ();
}

// --------------------------------------------------------------------------------
gboolean Stage_c::Locked ()
{
  return _locked;
}

// --------------------------------------------------------------------------------
void Stage_c::SetPrevious (Stage_c *previous)
{
  _previous = previous;
}

// --------------------------------------------------------------------------------
Stage_c *Stage_c::GetPreviousStage ()
{
  return _previous;
}

// --------------------------------------------------------------------------------
void Stage_c::Enter ()
{
}

// --------------------------------------------------------------------------------
void Stage_c::Wipe ()
{
}

// --------------------------------------------------------------------------------
void Stage_c::Load (xmlDoc *doc)
{
}

// --------------------------------------------------------------------------------
void Stage_c::Save (xmlTextWriter *xml_writer)
{
}

// --------------------------------------------------------------------------------
GSList *Stage_c::GetResult ()
{
  return _result;
}
