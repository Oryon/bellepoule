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

#ifdef WIRING_PI
#include <wiringPi.h>
#endif

#include "light.hpp"

// --------------------------------------------------------------------------------
Light::Light (GtkWidget          *w,
              Gpio::EventHandler  event_handler,
              void               *context,
              ...)
  : Object ("Light")
{
  _widget   = w;
  _pin_list = NULL;

  _event_handler = event_handler;
  _context       = context;

  {
    va_list ap;

    va_start (ap, context);
    while (1)
    {
      gchar *state;
      gchar *color;

      state = va_arg (ap, gchar *);
      if (state == NULL)
      {
        break;
      }
      color = va_arg (ap, char *);

      {
        GdkRGBA *rgba_color = g_new (GdkRGBA, 1);

        gdk_rgba_parse (rgba_color,
                        color);

        SetData (this,
                 state,
                 rgba_color,
                 g_free);
      }
    }
    va_end (ap);
  }
}

// --------------------------------------------------------------------------------
Light::~Light ()
{
  g_list_free_full (_pin_list,
                    (GDestroyNotify) Object::TryToRelease);
}

// --------------------------------------------------------------------------------
void Light::WireGpioPin (const gchar *state,
                         guint        pin_id)
{
  Gpio *gpio_pin = new Gpio (pin_id,
                             _event_handler,
                             _context,
                             INT_EDGE_BOTH,
                             TRUE);

  gpio_pin->SetData (this,
                     "light_state",
                     g_strdup (state),
                     g_free);

  _pin_list = g_list_prepend (_pin_list,
                              gpio_pin);
}

// --------------------------------------------------------------------------------
void Light::Refresh (GQuark  quark,
                     Light  *light)
{
  GList *current = light->_pin_list;

  while (current)
  {
    Gpio *gpio_pin = (Gpio *) current->data;

    if (gpio_pin->GetVoltageState () == 0)
    {
      light->SwitchOn ((const gchar *) gpio_pin->GetPtrData (light,
                                                             "light_state"));
      return;
    }
    current = g_list_next (current);
  }

  light->SwitchOff ();
}

// --------------------------------------------------------------------------------
void Light::SwitchOn (const gchar *state)
{
  GdkRGBA *color;

  if (state)
  {
    color = (GdkRGBA *) GetPtrData (this, state);
  }
  else
  {
    color = (GdkRGBA*) GetPtrData (this, "on");
  }

  if (color)
  {
    gtk_widget_override_background_color (_widget,
                                          GTK_STATE_FLAG_NORMAL,
                                          color);
  }
}

// --------------------------------------------------------------------------------
void Light::SwitchOff ()
{
  GdkRGBA *color = (GdkRGBA *) GetPtrData (this, "off");

  if (color)
  {
    gtk_widget_override_background_color (_widget,
                                          GTK_STATE_FLAG_NORMAL,
                                          color);
  }
  else
  {
    GdkRGBA default_color;

    default_color.alpha = 0;
    gtk_widget_override_background_color (_widget,
                                          GTK_STATE_FLAG_NORMAL,
                                          &default_color);
  }
}
