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
    Light (GtkWidget          *w,
           Gpio::EventHandler  event_handler,
           void               *context,
           ...);

    static void Refresh (GQuark  key_id,
                         Light  *light);

    void WireGpioPin (const gchar *state,
                      guint        pin_id);

  private:
    Gpio::EventHandler  _event_handler;
    void               *_context;
    GtkWidget          *_widget;
    GList              *_pin_list;

    virtual ~Light ();

    void SwitchOn (const gchar *state = NULL);

    void SwitchOff ();

};

#endif
