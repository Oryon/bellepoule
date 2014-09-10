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

#ifndef light_hpp
#define light_hpp

#include <gtk/gtk.h>

#include "util/object.hpp"
#include "gpio.hpp"

class Light : public Object
{
  public:
    Light (GtkWidget *w,
           guint      pin,
           ...);

    static void SetEventHandler (Gpio::EventHandler handler);

    static void Refresh (GQuark  key_id,
                         Light  *light);

    void SwitchOn (const gchar *state = NULL);

    void SwitchOff ();

  private:
    static Gpio::EventHandler  _event_handler;
    GtkWidget                 *_widget;
    Gpio                      *_pin;

    virtual ~Light ();
};

#endif
