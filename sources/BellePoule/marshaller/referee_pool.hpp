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

#ifndef referee_pool_hpp
#define referee_pool_hpp

#include <gtk/gtk.h>

#include "actors/referees_list.hpp"

namespace Marshaller
{
  class Job;
  class Slot;
  class EnlistedReferee;

  class RefereePool : public Object
  {
    public:
      RefereePool ();

      void ManageList (People::RefereesList *list);

      void ManageReferee (Net::Message *message);

      EnlistedReferee *GetReferee (guint ref);

      EnlistedReferee *GetRefereeFor (Job  *job,
                                      Slot *slot);

      void RefreshWorkload (const gchar *);

      void Spread ();

      GList *GetList ();

    private:
      GList *_list_by_weapon;

      ~RefereePool ();

      static void SpreadWeapon (People::RefereesList *list,
                                RefereePool  *rp);
  };
}

#endif
