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

#pragma once

#include <gtk/gtk.h>

#include "actors/checkin.hpp"

namespace People
{
  class RefereesList;
}

namespace Marshaller
{
  class Job;
  class Slot;
  class EnlistedReferee;

  class RefereePool : public Object,
                      public People::Checkin::Listener
  {
    public:
      RefereePool ();

      void ManageList (People::RefereesList *list);

      void ManageReferee (Net::Message *message);

      EnlistedReferee *GetReferee (guint ref);

      gboolean WeaponHasReferees (const gchar *weapon);

      void RefreshWorkload (const gchar *);

      void Spread ();

      void ExpandAll ();

      void CollapseAll ();

      People::RefereesList *GetListOf (const gchar *weapon);

      void SetDndPeerListener (DndConfig::Listener *listener);

    private:
      GList *_list_by_weapon;

      ~RefereePool ();

      gboolean OnPlayerListRowActivated (People::Checkin *checkin);

      static void OnDisplayJobs (GtkWidget            *w,
                                 People::RefereesList *referee_list);

      static void SpreadWeapon (People::RefereesList *list,
                                RefereePool          *rp);
  };
}
