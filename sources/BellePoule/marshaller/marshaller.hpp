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

#include "util/module.hpp"
#include "actors/referees_list.hpp"
#include "referee_pool.hpp"
#include "hall.hpp"

namespace Marshaller
{
  class Marshaller :
    public Module,
    public People::RefereesList::Listener,
    public Hall::Listener
  {
    public:
      Marshaller ();

      void Start ();

      gboolean OnHttpPost (Net::Message *message);

      void OnExposeWeapon (const gchar *weapon_code);

      void OnRefereeListCollapse ();

    private:
      Hall        *_hall;
      RefereePool *_referee_pool;
      GtkNotebook *_referee_notebook;

      virtual ~Marshaller ();

      void OnEvent (const gchar *event);

      void OnBlur ();

      void OnUnBlur ();

      void OnOpenCheckin (People::RefereesList *referee_list);
  };
}
