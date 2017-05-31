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

#include "util/canvas_module.hpp"

namespace Marshaller
{
  class Batch;

  class Timeline : public CanvasModule
  {
    public:
      struct Listener
      {
        virtual void OnTimelineCursorMoved () = 0;
      };

    public:
      Timeline (Listener *listener);

      void AddBatch (Batch *batch);

      void RemoveBatch (Batch *batch);

      void SetCursorTime (GDateTime *time);

      GDateTime *RetreiveCursorTime ();

      gboolean Redraw ();

      static gboolean RedrawCbk (Timeline *tl);

    private:
      static const guint     HOURS_MONITORED = 10;
      static const GTimeSpan STEP            = 15*G_TIME_SPAN_MINUTE;
      static const GTimeSpan START_MARGING   = STEP;

      GList         *_batch_list;
      GTimeSpan      _cursor;
      gdouble        _drag_start;
      GtkAllocation  _allocation;
      gdouble        _time_scale;
      gdouble        _batch_scale;
      GDateTime     *_origin;
      guint          _redraw_timeout;
      GooCanvasItem *_goo_cursor;
      GooCanvasItem *_goo_cursor_time;
      Listener      *_listener;

      ~Timeline ();

      void OnPlugged ();

      void DrawTimes ();

      void DrawCursors ();

      void DrawSlots ();

      void TranslateCursor (gdouble delta);

      void RefreshCursorTime ();

      static gboolean OnButtonPress (GooCanvasItem  *item,
                                     GooCanvasItem  *target,
                                     GdkEventButton *event,
                                     Timeline       *tl);

      static gboolean OnButtonRelease (GooCanvasItem  *item,
                                       GooCanvasItem  *target,
                                       GdkEventButton *event,
                                       Timeline       *tl);

      static gboolean OnMotion (GooCanvasItem  *item,
                                GooCanvasItem  *target_item,
                                GdkEventMotion *event,
                                Timeline       *tl);
  };
}
