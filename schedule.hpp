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

#ifndef schedule_hpp
#define schedule_hpp

#include <libxml/xmlwriter.h>
#include "stage.hpp"
#include "module.hpp"

class Schedule_c : public Module_c
{
  public:
    typedef void (Object_c::*StageEvent_t) ();

  public:
     Schedule_c ();

    void DisplayList ();

    void AddStage    (Stage_c *stage);
    void RemoveStage (Stage_c *stage);

    void Save          (xmlTextWriter *xml_writer);
    void Load          (xmlDoc        *doc);

    void on_previous_stage_toolbutton_clicked ();
    void on_next_stage_toolbutton_clicked     ();
    void on_stage_selected ();

  private:
    GtkListStore *_list_store;
    GList        *_stage_list;
    GList        *_current_stage;
    Module_c     *_selected_module;

    void   SetCurrentStage    (GList *stage);
    void   CancelCurrentStage ();

    static void on_row_selected (GtkWidget *widget,
                                 GdkEvent  *event,
                                 gpointer  *data);

  private:
    GtkWidget *_formula_dlg;

    ~Schedule_c ();
    void OnPlugged ();
};

#endif
