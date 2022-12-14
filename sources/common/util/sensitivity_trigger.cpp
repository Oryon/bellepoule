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

#include <stdlib.h>
#include <string.h>

#include "sensitivity_trigger.hpp"

const gchar *SensitivityTrigger::_data_key = "SensitivityTrigger::lock";

// --------------------------------------------------------------------------------
SensitivityTrigger::SensitivityTrigger ()
  : Object ("SensitivityTrigger")
{
  _widget_list = nullptr;
}

// --------------------------------------------------------------------------------
SensitivityTrigger::~SensitivityTrigger ()
{
  g_slist_free (_widget_list);
}

// --------------------------------------------------------------------------------
void SensitivityTrigger::AddWidget (GtkWidget *w)
{
  if (w)
  {
    _widget_list = g_slist_append (_widget_list,
                                   w);

    g_signal_connect (G_OBJECT (w), "realize",
                      G_CALLBACK (on_realize), this);
  }
}

// --------------------------------------------------------------------------------
void SensitivityTrigger::SwitchOn ()
{
  GSList *current = _widget_list;

  while (current)
  {
    GtkWidget *w    = GTK_WIDGET (current->data);
    gint       lock = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (w), _data_key));

    lock--;
    g_object_set_data (G_OBJECT (w),
                       _data_key,
                       (void *) lock);

    if (lock == 0)
    {
      gtk_widget_set_sensitive (w,
                                TRUE);
    }
    current = g_slist_next (current);
  }
}

// --------------------------------------------------------------------------------
void SensitivityTrigger::SwitchOff ()
{
  GSList *current = _widget_list;

  while (current)
  {
    GtkWidget *w    = GTK_WIDGET (current->data);
    gint       lock = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (w), _data_key));

    lock++;
    g_object_set_data (G_OBJECT (w),
                       _data_key,
                       (void *) lock);

    if (lock == 1)
    {
      gtk_widget_set_sensitive (w,
                                FALSE);
    }
    current = g_slist_next (current);
  }
}

// --------------------------------------------------------------------------------
void SensitivityTrigger::SetSensitivity (GtkWidget *w)
{
  gint lock = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (w), _data_key));

  if (lock > 0)
  {
    gtk_widget_set_sensitive (w,
                              FALSE);
  }
  else
  {
    gtk_widget_set_sensitive (w,
                              TRUE);
  }
}

// --------------------------------------------------------------------------------
void SensitivityTrigger::on_realize (GtkWidget          *widget,
                                     SensitivityTrigger *trigger)
{
  trigger->SetSensitivity (widget);
}
