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

    void CreateDefault ();

    void AddStage    (Stage_c *stage,
                      Stage_c *after);
    void RemoveStage (Stage_c *stage);

    void Save          (xmlTextWriter *xml_writer);
    void Load          (xmlDoc        *doc);

    Stage_c *GetStage (guint index);

    void on_previous_stage_toolbutton_clicked ();
    void on_next_stage_toolbutton_clicked     ();
    void on_stage_selected ();
    void on_stage_removed ();

  protected:
    void RefreshSensitivity ();

  private:
    GtkListStore *_list_store;
    GList        *_stage_list;
    guint         _current_stage;

    void      SetCurrentStage    (guint index);
    Module_c *GetSelectedModule  ();

    gint GetNotebookPageNum (Stage_c *stage);

    void AddStage (Stage_c *stage);

    static void OnStageStatusUpdated (Stage_c    *stage,
                                      Schedule_c *schedule);

    static void on_row_selected (GtkWidget *widget,
                                 GdkEvent  *event,
                                 gpointer  *data);

    static gboolean on_new_stage_selected (GtkWidget      *widget,
                                           GdkEventButton *event,
                                           Schedule_c     *owner);

  private:
    GtkWidget *_formula_dlg;

    ~Schedule_c ();
    void OnPlugged ();
    void PlugStage (Stage_c *stage);
};

#endif
