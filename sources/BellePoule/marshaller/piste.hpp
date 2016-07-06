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

#ifndef piste_hpp
#define piste_hpp

#include <goocanvas.h>

#include "util/drop_zone.hpp"
#include "timeslot.hpp"

class Module;

namespace Marshaller
{
  class Job;
  class Batch;
  class EnlistedReferee;

  class Piste :
    public DropZone,
    public TimeSlot::Owner
  {
    public:
      struct Listener
      {
        virtual void OnPisteButtonEvent (Piste          *piste,
                                         GdkEventButton *event) = 0;
        virtual void OnPisteMotionEvent (Piste          *piste,
                                         GdkEventMotion *event) = 0;
      };

    public:
      Piste (GooCanvasItem *parent,
             Module        *container);

      void ConvertFromPisteSpace (gdouble *x,
                                  gdouble *y);

      void SetListener (Listener *listener);

      GList *GetFreeSlots (GDateTime *from,
                           GTimeSpan  duration);

      void AddReferee (EnlistedReferee *referee);

      void RemoveBatch (Batch *batch);

      void Disable ();

      gboolean Overlaps (GooCanvasBounds *rectangle);

      void Select ();

      void UnSelect ();

      void DisplayAtTime (GDateTime *time);

      void Translate (gdouble tx,
                      gdouble ty);

      void Rotate ();

      guint GetId ();

      void SetId (guint id);

      void AlignOnGrid ();

      void AnchorTo (Piste *piste);

      void GetHorizontalCoord (gdouble *x,
                               gdouble *y);

      static const gdouble _W;
      static const gdouble _H;
      static const gdouble _BORDER_W;
      static const gdouble _RESOLUTION;

      Piste::Listener *_listener;
      GooCanvasItem   *_root_item;
      GooCanvasItem   *_rect_item;
      GooCanvasItem   *_progress_item;
      GooCanvasItem   *_id_item;
      GooCanvasItem   *_title_item;
      GooCanvasItem   *_referee_table;
      GooCanvasItem   *_referee_name;
      guint            _id;
      gboolean         _horizontal;
      gchar           *_color;
      gchar           *_focus_color;
      GList           *_timeslots;
      GDateTime       *_display_time;

      ~Piste ();

      void Focus ();

      void Unfocus ();

      void SetColor (const gchar *color);

      void MonitorEvent (GooCanvasItem *item);

      gdouble GetGridAdjustment (gdouble coordinate);

      void OnObjectDeleted (Object *object);

      void CleanDisplay ();

      void OnSlotUpdated (TimeSlot *timeslot);

      void  OnSlotLocked  (TimeSlot *timeslot);

      gboolean TimeIsInTimeslot (GDateTime *time,
                                 TimeSlot  *timeslot);

      TimeSlot *GetTimeslotAt (GDateTime *time);

      static gboolean OnButtonPress (GooCanvasItem  *item,
                                     GooCanvasItem  *target,
                                     GdkEventButton *event,
                                     Piste          *piste);

      static gboolean OnButtonRelease (GooCanvasItem  *item,
                                       GooCanvasItem  *target,
                                       GdkEventButton *event,
                                       Piste          *piste);

      static gboolean OnMotionNotify (GooCanvasItem  *item,
                                      GooCanvasItem  *target,
                                      GdkEventMotion *event,
                                      Piste          *piste);

      static gint CompareJob (Job *a,
                              Job *b);
  };
}

#endif
