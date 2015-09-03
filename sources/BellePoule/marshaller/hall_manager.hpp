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

#ifndef hall_manager_hpp
#define hall_manager_hpp

#include "util/module.hpp"
#include "actors/referees_list.hpp"

class Hall;

class HallManager : public Module
{
  public:
    HallManager ();

    void Start ();

    void OnHttpPost (Net::Message *message);

  private:
    Hall                 *_hall;
    People::RefereesList *_referee_list;

    virtual ~HallManager ();

    void ManageReferee (const gchar *data);
};

#endif
