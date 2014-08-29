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

#include "light.hpp"

// --------------------------------------------------------------------------------
Light::Light (GtkWidget *w,
              ...)
  : Object ("Light")
{
  _widget = w;

  {
    va_list ap;

    va_start (ap, w);
    while (1)
    {
      gchar *state;
      gchar *color;

      state = va_arg (ap, char *);
      if (state == NULL)
      {
        break;
      }

      color = va_arg (ap, char *);
      if (color == NULL)
      {
        break;
      }

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

  SwitchOff ();
}

// --------------------------------------------------------------------------------
Light::~Light ()
{
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
