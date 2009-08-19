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

#include "pools_list.hpp"
#include "pool_supervisor.hpp"

#include "stage.hpp"

#if 0
  {
    GtkWidget *note_book = _glade->GetWidget ("stage_notebook");

    if (_schedule->GetCurrentStage () == Schedule_c::POOL_ALLOCATION)
    {
      _pools_list = new PoolsList_c (_players_base);

      Plug (_pools_list,
            _glade->GetWidget ("pools_list_hook"));

      _players_base->Lock ();
      _pools_list->Allocate ();
    }
    else if (_schedule->GetCurrentStage () == Schedule_c::POOL)
    {
      _pool_supervisor = new PoolSupervisor_c ();

      Plug (_pool_supervisor,
            _glade->GetWidget ("pool_hook"));

      for (guint p = 0; p < _pools_list->GetNbPools (); p++)
      {
        _pool_supervisor->Manage (_pools_list->GetPool (p));
      }

      gtk_notebook_set_current_page  (GTK_NOTEBOOK (note_book),
                                      1);
    }
  }
#endif

#if 0
  {
    GtkWidget *note_book = _glade->GetWidget ("stage_notebook");

    if (_schedule->GetCurrentStage () == Schedule_c::CHECKIN)
    {
      Object_c::Release (_pools_list);
      _pools_list = NULL;
    }
    else if (_schedule->GetCurrentStage () == Schedule_c::POOL_ALLOCATION)
    {
      Object_c::Release (_pool_supervisor);
      _pool_supervisor = NULL;

      gtk_notebook_set_current_page  (GTK_NOTEBOOK (note_book),
                                      0);
    }
  }
#endif

// --------------------------------------------------------------------------------
Stage_c::Stage_c (gchar *name)
{
  _name   = name;
  _locked = false;
}

// --------------------------------------------------------------------------------
Stage_c::~Stage_c ()
{
}

// --------------------------------------------------------------------------------
gchar *Stage_c::GetName ()
{
  return _name;
}

// --------------------------------------------------------------------------------
void Stage_c::Lock ()
{
  _locked = true;
  OnLocked ();
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
