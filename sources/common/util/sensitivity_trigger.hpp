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

#include "object.hpp"

class SensitivityTrigger : public virtual Object
{
  public:
    SensitivityTrigger ();

    virtual ~SensitivityTrigger ();

    void AddWidget (GtkWidget *w);

    void SwitchOn ();

    void SwitchOff ();

    static void on_realize (GtkWidget          *widget,
                            SensitivityTrigger *trigger);

  private:
    static const gchar *_data_key;

    GSList *_widget_list;

    void SetSensitivity (GtkWidget *w);
};
