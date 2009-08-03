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

#ifndef module_hpp
#define module_hpp

#include <gtk/gtk.h>

#include "object.hpp"
#include "schedule.hpp"
#include "glade.hpp"

class Module_c : public Object_c
{
  public:
    virtual ~Module_c ();

    void RegisterSchedule (Schedule_c *schedule);

    void Plug (Module_c  *module,
               GtkWidget *in);
    void UnPlug ();

  protected:
    Schedule_c *_schedule;
    Glade_c    *_glade;

    Module_c (gchar *glade_file,
              gchar *root = NULL);

    virtual void OnPlugged   ();
    virtual void OnUnPlugged ();

    GtkWidget *GetRootWidget ();

    void AddSensitiveWidget (GtkWidget *w);
    void EnableSensitiveWidgets ();
    void DisableSensitiveWidgets ();

  private:
    GtkWidget *_root;
    GSList    *_sensitive_widgets;
    GSList    *_plugged_list;
    Module_c  *_owner;

    Module_c ();
};

#endif
