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
#include "object.hpp"

class Schedule_c : public Object_c
{
  public:
    typedef enum
    {
      CHECKIN,
      POOL_ALLOCATION,
      POOL,
      TABLE,
      OVER,

      NB_STAGE
    } stage_t;

    typedef void (Object_c::*StageEvent_t) ();

  public:
     Schedule_c (GtkWidget *stage_name_widget,
                 GtkWidget *previous_widget,
                 GtkWidget *next_widget);
    ~Schedule_c ();

    void     Start           ();

    void     NextStage       ();
    void     PreviousStage   ();
    stage_t  GetCurrentStage ();

    void     Save            (xmlTextWriter *xml_writer);
    void     Load            (xmlNode       *xml_node);

    void     Subscribe       (Object_c     *client,
                              stage_t       stage,
                              StageEvent_t  stage_entered,
                              StageEvent_t  stage_leaved);
    void     UnSubscribe     (Object_c *client);

  private:
    typedef struct
    {
      Object_c     *client;
      StageEvent_t  OnStageEntered;
      StageEvent_t  OnStageLeaved;
    } Subscriber_t;

    stage_t    _current_stage;
    GSList    *_subscriber[NB_STAGE];
    GtkWidget *_stage_name_widget;
    GtkWidget *_previous_widget;
    GtkWidget *_next_widget;

    void   SetCurrentStage    (stage_t stage);
    void   CancelCurrentStage ();
    gchar *GetStageName       (stage_t stage);
};

#endif
