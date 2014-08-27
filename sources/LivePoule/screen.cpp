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

#include "screen.hpp"

// --------------------------------------------------------------------------------
Screen::Screen ()
  : Module ("LivePoule.glade")
{
  GtkWidget *window       = _glade->GetRootWidget ();
  gint       glade_width;
  gint       glade_height;

  // Show the main window
  {
    gtk_window_fullscreen (GTK_WINDOW (window));
    gtk_widget_show_all (window);

    gtk_window_get_size (GTK_WINDOW (window),
                         &glade_width,
                         &glade_height);
  }

  // Let fit it the screen
  {
    GdkDisplay *display     = gdk_display_get_default ();
    GdkScreen  *screen      = gdk_display_get_default_screen (display);
    gint        full_width  = gdk_screen_get_width (screen);
    gint        full_height = gdk_screen_get_height (screen);

    Rescale (MIN (full_width/glade_width, full_height/glade_height));
  }

  // Colorizes the lights
  {
    {
      GdkRGBA green = {0.137, 0.568, 0.137, 1.0};

      gtk_widget_override_background_color (_glade->GetWidget ("green_light_viewport"),
                                            GTK_STATE_FLAG_NORMAL,
                                            &green);
    }
    {
      GdkRGBA red = {0.568, 0.137, 0.137, 1.0};

      gtk_widget_override_background_color (_glade->GetWidget ("red_light_viewport"),
                                            GTK_STATE_FLAG_NORMAL,
                                            &red);
    }
    {
      GdkRGBA purple = {(double) 0xAD / 255.0, (double) 0xA7 / 255.0, (double) 0xC8 / 255.0, 1.0};

      gtk_widget_override_background_color (_glade->GetWidget ("title_viewport"),
                                            GTK_STATE_FLAG_NORMAL,
                                            &purple);
    }
  }
}

// --------------------------------------------------------------------------------
Screen::~Screen ()
{
}

// --------------------------------------------------------------------------------
void Screen::Rescale (gdouble factor)
{
  GSList *objects = _glade->GetObjectList ();
  GSList *current = objects;

  while (current)
  {
    GObject *object = (GObject *) current->data;

    if (GTK_IS_LABEL (object))
    {
      GtkLabel          *label      = GTK_LABEL (object);
      PangoAttrList     *attr_list  = pango_attr_list_copy (gtk_label_get_attributes (label));
      PangoAttrIterator *iter       = pango_attr_list_get_iterator (attr_list);
      PangoAttrFloat    *scale_attr = (PangoAttrFloat *) pango_attr_iterator_get (iter, PANGO_ATTR_SCALE);

      scale_attr->value *= factor;
      gtk_label_set_attributes (label,
                                attr_list);

      pango_attr_iterator_destroy (iter);
      pango_attr_list_unref (attr_list);
    }

    current = g_slist_next (current);
  }

  g_slist_free (objects);
}

// --------------------------------------------------------------------------------
void Screen::Unfullscreen ()
{
  GtkWidget *w = _glade->GetRootWidget ();

  gtk_window_unfullscreen (GTK_WINDOW (w));
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT gboolean on_root_key_press_event (GtkWidget   *widget,
                                                             GdkEventKey *event,
                                                             Object      *owner)
{
  if (event->keyval == GDK_KEY_Escape)
  {
    Screen *s = dynamic_cast <Screen *> (owner);

    s->Unfullscreen ();
  }

  return FALSE;
}

