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

#ifndef gpio_hpp
#define gpio_hpp

#include <gtk/gtk.h>

#include "util/object.hpp"

class Gpio : public Object
{
  public:
    typedef void (*EventHandler) () ;

  public:
    static void Init ();

    Gpio (guint        pin,
          EventHandler handler);

    guint GetState ();

  private:
    EventHandler _event_handler;
    guint        _pin;

    virtual ~Gpio ();
};

#endif
