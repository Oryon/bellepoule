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

#include "util/dnd_config.hpp"
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
  class Timeline;

  class RefereePool : public Object,
                      public People::Checkin::Listener
  {
    public:
      RefereePool ();

      void ManageList (People::RefereesList *list);

      void ManageReferee (Net::Message *message);

      void ManageHandShake (Net::Message *message);

      EnlistedReferee *GetReferee (guint ref);

      gboolean WeaponHasReferees (const gchar *weapon);

      void RefreshWorkload (const gchar *weapon_code);

      void RefreshAvailability (Timeline *timeline,
                                GList    *pistes);

      void Spread () override;

      void ExpandAll ();

      void CollapseAll ();

      People::RefereesList *GetListOf (const gchar *weapon);

      void SetDndPeerListener (DndConfig::Listener *listener);

      void UpdateAffinities (Player *referee);

    private:
      GList    *_list_by_weapon;
      Timeline *_timeline;
      GList    *_piste_list;
      GdkColor *_free_color;

      ~RefereePool () override;

      gboolean OnPlayerListRowActivated (People::Checkin *checkin) override;

      static void OnDisplayJobs (GtkWidget            *widget,
                                 People::RefereesList *referee_list);

      static void SetRefereeAvailability (People::RefereesList *referee_list,
                                          gboolean              available);

      static void OnMakeRefereeAvailable (GtkWidget            *widget,
                                          People::RefereesList *referee_list);

      static void OnMakeRefereeNotAvailable (GtkWidget            *widget,
                                             People::RefereesList *referee_list);

      static void SpreadWeapon (People::RefereesList *list,
                                RefereePool          *rp);

      static void RefreshWeaponAvailability (People::RefereesList *list,
                                             RefereePool          *rp);
  };
}
