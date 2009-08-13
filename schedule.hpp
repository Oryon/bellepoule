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
#include "object.hpp"

class Schedule_c : public Object_c
{
  public:
    typedef void (Object_c::*StageEvent_t) ();

  public:
     Schedule_c (GtkWidget *stage_name_widget,
                 GtkWidget *previous_widget,
                 GtkWidget *next_widget,
                 GtkWidget *notebook_widget);
    ~Schedule_c ();

    void     Start           ();

    void     AddStage        (Stage_c *stage);
    void     RemoveStage     (Stage_c *stage);
    void     NextStage       ();
    void     PreviousStage   ();

    void     Save            (xmlTextWriter *xml_writer);
    void     Load            (xmlNode       *xml_node);

  private:
    GtkWidget *_stage_name_widget;
    GtkWidget *_previous_widget;
    GtkWidget *_next_widget;
    GtkWidget *_notebook_widget;
    GList     *_stage_list;
    GList     *_current_stage;

    void   SetCurrentStage    (GList *stage);
    void   CancelCurrentStage ();
};

#endif
