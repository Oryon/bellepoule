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
#include <util/object.hpp>

namespace Swiss
{
  class WheelOfFortune : public Object
  {
    public:
      WheelOfFortune (GSList *list);

      void *Turn ();

      void *TryAgain ();

    private:
      GSList *_list;
      GSList *_origin;
      GSList *_position;
      guint   _size;
      GRand  *_randomizer;

      ~WheelOfFortune ();
  };
}
