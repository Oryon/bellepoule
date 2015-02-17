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
#include "application/weapon.hpp"
#include "hall.hpp"
#include "batch.hpp"

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
    GtkNotebook *notebook = GTK_NOTEBOOK (_glade->GetWidget ("referee_notebook"));
    GSList      *current  = Weapon::GetList ();

    while (current)
    {
      Weapon               *weapon   = (Weapon *) current->data;
      People::RefereesList *list     = new People::RefereesList (NULL);
      GtkWidget            *viewport = gtk_viewport_new (NULL, NULL);

      gtk_notebook_append_page (notebook,
                                viewport,
                                gtk_label_new (gettext (weapon->GetImage ())));
      Plug (list,
            viewport);

      current = g_slist_next (current);
    }
  }

  {
    GtkWidget *notebook = _glade->GetWidget ("batch_notebook");
    Batch     *batch;

    batch = new Batch ();
    batch->AttachTo (GTK_NOTEBOOK (notebook));

    batch = new Batch ();
    batch->AttachTo (GTK_NOTEBOOK (notebook));
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
