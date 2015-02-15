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

#include "people_management/referees_list.hpp"
#include "hall.hpp"

#include "hall_manager.hpp"

// --------------------------------------------------------------------------------
HallManager::HallManager ()
  : Module ("hall_manager.glade")
{
  {
    Hall *hall = new Hall ();

    Plug (hall,
          _glade->GetWidget ("hall_viewport"));
  }

  {
    People::RefereesList *list = new People::RefereesList (NULL);

    Plug (list,
          _glade->GetWidget ("referees_viewport"));

#if 0
    {
      _notebook = GTK_NOTEBOOK (to);

      gtk_notebook_append_page (_notebook,
                                GetRootWidget (),
                                _glade->GetWidget ("notebook_title"));

      gtk_notebook_set_tab_reorderable (_notebook,
                                        GetRootWidget (),
                                        TRUE);
    }
#endif
  }
}

// --------------------------------------------------------------------------------
HallManager::~HallManager ()
{
}

// --------------------------------------------------------------------------------
void HallManager::Start ()
{
}
