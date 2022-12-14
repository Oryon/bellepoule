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

#include "batch.hpp"
#include "competition.hpp"
#include "slot.hpp"
#include "job.hpp"
#include "clock.hpp"
#include "timeline.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  Timeline::Timeline (Clock    *clock,
                      Listener *listener)
    : Object ("Timeline"),
    CanvasModule ("timeline.glade", "canvas_scrolled_window")
  {
    _competition_list = nullptr;
    _listener         = listener;
    _button_pressed   = FALSE;

    _clock = clock;
    _clock->Retain ();

    {
      GDateTime *now = _clock->RetreiveNow ();

      _origin = g_date_time_add_full (now,
                                      0,
                                      0,
                                      0,
                                      0,
                                      -g_date_time_get_minute  (now) - START_MARGING/G_TIME_SPAN_MINUTE,
                                      -g_date_time_get_seconds (now));

      _cursor = g_date_time_difference (now,
                                        _origin);
      _cursor += STEP;
      _cursor -= _cursor % STEP;

      g_date_time_unref (now);
    }
  }

  // --------------------------------------------------------------------------------
  Timeline::~Timeline ()
  {
    g_list_free       (_competition_list);
    g_date_time_unref (_origin);
    _clock->Release ();
  }

  // --------------------------------------------------------------------------------
  void Timeline::OnPlugged ()
  {
    CanvasModule::OnPlugged ();

    g_object_set (G_OBJECT (GetCanvas ()),
                  "bounds-padding",     0.0,
                  "bounds-from-origin", TRUE,
                  NULL);

    g_signal_connect (GetRootItem (), "button_press_event",
                      G_CALLBACK (OnBgButtonPress), this);
    g_signal_connect (GetRootItem (), "button_release_event",
                      G_CALLBACK (OnBgButtonRelease), this);
    g_signal_connect (GetRootItem (), "motion-notify-event",
                      G_CALLBACK (OnBgMotion), this);
  }

  // --------------------------------------------------------------------------------
  void Timeline::AddCompetition (Competition *competition)
  {
    _competition_list = g_list_append (_competition_list,
                                       competition);
  }

  // --------------------------------------------------------------------------------
  void Timeline::RemoveCompetition (Competition *competition)
  {
    _competition_list = g_list_remove (_competition_list,
                                       competition);
  }

  // --------------------------------------------------------------------------------
  void Timeline::DrawTimes ()
  {
    gint origin_hh  = g_date_time_get_hour (_origin);
    gint origin_mm  = g_date_time_get_minute (_origin) + g_date_time_get_second (_origin)/60;
    gint first_mark = origin_mm;
    gint offset_mm;

    if (origin_mm%15)
    {
      first_mark = origin_mm + 15 - origin_mm%15;
    }
    offset_mm = first_mark - origin_mm;

    for (guint mark = 0; mark <= HOURS_MONITORED*4; mark++)
    {
      GTimeSpan x_span = (offset_mm + mark*15)*G_TIME_SPAN_MINUTE;

      if ((mark*15 + first_mark) % 60 == 0)
      {
        guint  hh   = (origin_hh + (mark*15 + first_mark)/60) % 24;
        gchar *time = g_strdup_printf ("<span background=\"white\">%02d:00</span>", hh);

        time = g_strdup_printf ("<span background=\"white\">%02d:00</span>", hh);

        goo_canvas_rect_new (GetRootItem (),
                             x_span * _time_scale,
                             0.7 * _competition_scale,
                             0.5,
                             0.3 * _competition_scale,
                             "fill-color",     "black",
                             "stroke-pattern", NULL,
                             "line-width",     0.0,
                             "pointer-events", GOO_CANVAS_EVENTS_NONE,
                             NULL);
        goo_canvas_text_new (GetRootItem (),
                             time,
                             x_span * _time_scale,
                             _competition_scale,
                             -1.0,
                             GTK_ANCHOR_S,
                             "fill-color",     "grey",
                             "font",           BP_FONT "10px",
                             "use-markup",     TRUE,
                             "pointer-events", GOO_CANVAS_EVENTS_NONE,
                             NULL);
        g_free (time);
      }
      else
      {
        goo_canvas_rect_new (GetRootItem (),
                             x_span * _time_scale,
                             0.95 * _competition_scale,
                             0.3,
                             0.05 * _competition_scale,
                             "fill-color",     "black",
                             "stroke-pattern", NULL,
                             "line-width",     0.0,
                             "pointer-events", GOO_CANVAS_EVENTS_NONE,
                             NULL);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Timeline::DrawCursors ()
  {
    {
      GDateTime *now = _clock->RetreiveNow ();
      GTimeSpan  x   = g_date_time_difference (now, _origin);

      goo_canvas_rect_new (GetRootItem (),
                           0.0,
                           0.0,
                           x * _time_scale,
                           _competition_scale,
                           "fill-color-rgba", 0x0000000F,
                           "stroke-pattern", NULL,
                           "line-width",     0.0,
                           "pointer-events", GOO_CANVAS_EVENTS_NONE,
                           NULL);
      goo_canvas_rect_new (GetRootItem (),
                           x * _time_scale,
                           0.0,
                           0.7,
                           _competition_scale,
                           "fill-color-rgba", 0x00000060,
                           "stroke-pattern", NULL,
                           "line-width",     0.0,
                           "pointer-events", GOO_CANVAS_EVENTS_NONE,
                           NULL);
      g_date_time_unref (now);
    }

    {
      _goo_cursor = goo_canvas_rect_new (GetRootItem (),
                                         _cursor * _time_scale,
                                         0.0,
                                         2.0,
                                         _competition_scale,
                                         "fill-color",     "black",
                                         "stroke-pattern", NULL,
                                         "line-width",     0.0,
                                         NULL);
      g_signal_connect (_goo_cursor, "button_press_event",
                        G_CALLBACK (OnButtonPress), this);

      _goo_cursor_time = goo_canvas_text_new (GetRootItem (),
                                              "",
                                              _cursor * _time_scale,
                                              _competition_scale/2.0,
                                              -1.0,
                                              GTK_ANCHOR_W,
                                              "font",       BP_FONT "10px",
                                              "use-markup", TRUE,
                                              NULL);
      RefreshCursorTime ();
      g_signal_connect (_goo_cursor_time, "button_press_event",
                        G_CALLBACK (OnButtonPress), this);
    }
  }

  // --------------------------------------------------------------------------------
  void Timeline::DrawSlots ()
  {
    for (guint layer = 0; layer < 2; layer++)
    {
      GList *current_competition = _competition_list;

      for (guint i = 0; current_competition != nullptr; i++)
      {
        Competition *competition   = (Competition *) current_competition->data;
        gchar       *color         = gdk_color_to_string (competition->GetColor ());
        GList       *current_batch = competition->GetBatches ();

        while (current_batch)
        {
          Batch *batch       = (Batch *) current_batch->data;
          GList *current_job = batch->GetScheduledJobs ();

          while (current_job != nullptr)
          {
            Job       *job   = (Job *) current_job->data;
            Slot      *slot  = job->GetSlot ();
            GDateTime *start = slot->GetStartTime ();
            gdouble    x     = g_date_time_difference (start, _origin) * _time_scale;
            gdouble    w     = (slot->GetDuration () - 30*G_TIME_SPAN_SECOND) * _time_scale;

            if (layer == 0)
            {
              goo_canvas_rect_new (GetRootItem (),
                                   x,
                                   i*_competition_scale/10.0,
                                   w,
                                   _competition_scale/10.0,
                                   "fill-color",     color,
                                   "stroke-pattern", NULL,
                                   "line-width",     1.0,
                                   "pointer-events", GOO_CANVAS_EVENTS_NONE,
                                   NULL);
            }
            else
            {
              const gdouble line_width = 1.0;

              goo_canvas_rect_new (GetRootItem (),
                                   x + line_width/2,
                                   i*_competition_scale/10.0 + line_width/2,
                                   w - line_width,
                                   _competition_scale/10.0 - line_width,
                                   "fill-pattern",   NULL,
                                   "line-width",     1.0,
                                   "pointer-events", GOO_CANVAS_EVENTS_NONE,
                                   NULL);
            }

            current_job = g_list_next (current_job);
          }
          current_batch = g_list_next (current_batch);
        }

        g_free (color);

        current_competition = g_list_next (current_competition);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Timeline::Redraw ()
  {
    Wipe ();

    {
      gtk_widget_get_allocation (GetRootWidget (),
                                 &_allocation);

      _competition_scale = (gdouble)_allocation.height;
      _time_scale        = (gdouble)_allocation.width
        / (HOURS_MONITORED*G_TIME_SPAN_HOUR + START_MARGING);
    }

    DrawSlots   ();
    DrawTimes   ();
    DrawCursors ();
  }

  // --------------------------------------------------------------------------------
  void Timeline::SlideOut (gdouble  position,
                           gboolean one_shot)
  {
    GDateTime *now = _clock->RetreiveNow ();
    GDateTime *new_origin;

    new_origin = g_date_time_add_full (_origin,
                                       0,
                                       0,
                                       0,
                                       0,
                                       0,
                                       ((_slideout_origin - position) / _time_scale) / G_TIME_SPAN_SECOND);

    if (one_shot)
    {
      _slideout_origin = 0;
    }
    else
    {
      _slideout_origin = position;
    }

    g_date_time_unref (_origin);
    _origin = new_origin;

    Redraw ();

    if (_listener)
    {
      _listener->OnTimelineCursorMoved ();
    }

    g_date_time_unref (now);
  }

  // --------------------------------------------------------------------------------
  gboolean Timeline::OnButtonPress (GooCanvasItem  *item,
                                    GooCanvasItem  *target,
                                    GdkEventButton *event,
                                    Timeline       *tl)
  {
    tl->_drag_start = event->x;

    g_signal_connect (G_OBJECT (item), "motion-notify-event",
                      G_CALLBACK (OnMotion), tl);
    g_signal_connect (G_OBJECT (item), "button_release_event",
                      G_CALLBACK (OnButtonRelease), tl);

    tl->SetCursor (GDK_FLEUR);

    return TRUE;
  }

  // --------------------------------------------------------------------------------
  gboolean Timeline::OnBgButtonPress (GooCanvasItem  *item,
                                      GooCanvasItem  *target,
                                      GdkEventButton *event,
                                      Timeline       *tl)
  {
    tl->SetCursor (GDK_FLEUR);

    tl->_button_pressed      = TRUE;
    tl->_slideout_origin     = event->x;
    tl->_button_press_origin = event->x;

    return TRUE;
  }

  // --------------------------------------------------------------------------------
  gboolean Timeline::OnBgButtonRelease (GooCanvasItem  *item,
                                        GooCanvasItem  *target,
                                        GdkEventButton *event,
                                        Timeline       *tl)
  {
    tl->SetCursor (GDK_ARROW);

    if (tl->_button_pressed)
    {
      if (tl->_button_press_origin == tl->_slideout_origin)
      {
        tl->TranslateCursor (event->x - (tl->_cursor * tl->_time_scale));

        {
          GTimeSpan rectification = tl->GetTimeRectification (tl->_cursor);

          tl->TranslateCursor (rectification * tl->_time_scale);
        }
      }
      else
      {
        // Round origin
        {
          GTimeSpan  rectification;
          GDateTime *new_origin;
          GTimeSpan  time_span = 0;

          time_span += g_date_time_get_minute  (tl->_origin) * G_TIME_SPAN_MINUTE;
          time_span += g_date_time_get_seconds (tl->_origin) * G_TIME_SPAN_SECOND;

          rectification = tl->GetTimeRectification (time_span);

          new_origin = g_date_time_add (tl->_origin,
                                        rectification);
          g_date_time_unref (tl->_origin);
          tl->_origin = new_origin;
        }

        // Round cursor
        {
          GDateTime *rounded = tl->RetreiveCursorTime (TRUE);

          tl->SetCursorTime (rounded);
          g_date_time_unref (rounded);
        }

        tl->Redraw ();
      }

      tl->_button_pressed      = FALSE;
      tl->_slideout_origin     = 0;
      tl->_button_press_origin = 0;

      if (tl->_listener)
      {
        tl->_listener->OnTimelineCursorMoved ();
        tl->_listener->OnTimelineCursorChanged ();
      }
    }

    return TRUE;
  }

  // --------------------------------------------------------------------------------
  gboolean Timeline::OnBgMotion (GooCanvasItem  *item,
                                 GooCanvasItem  *target_item,
                                 GdkEventMotion *event,
                                 Timeline       *tl)
  {
    if (tl->_button_pressed)
    {
      tl->SlideOut (event->x);
    }
    else
    {
      tl->SetCursor (GDK_ARROW);
    }

    return TRUE;
  }

  // --------------------------------------------------------------------------------
  GDateTime *Timeline::RetreiveCursorTime (gboolean rounded)
  {
    GTimeSpan cursor = _cursor;

    if (rounded)
    {
      cursor += GetTimeRectification (_cursor);
    }

    return g_date_time_add (_origin,
                            cursor);
  }

  // --------------------------------------------------------------------------------
  GTimeSpan Timeline::GetTimeRectification (GTimeSpan time)
  {
    GTimeSpan round = time % STEP;

    if (round > (STEP / 2))
    {
      return STEP - round;
    }

    return -round;
  }

  // --------------------------------------------------------------------------------
  gboolean Timeline::OnMotion (GooCanvasItem  *item,
                               GooCanvasItem  *target_item,
                               GdkEventMotion *event,
                               Timeline       *tl)
  {
    if (item)
    {
      gdouble drag_now = event->x;

      if (drag_now < 2.0)
      {
        drag_now = 2.0;
      }
      if (drag_now > tl->_allocation.width - 2.0)
      {
        drag_now = tl->_allocation.width - 2.0;
      }

      tl->TranslateCursor (drag_now - tl->_drag_start);

      return TRUE;
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void Timeline::SetCursorTime (GDateTime *time)
  {
    _cursor = g_date_time_difference (time,
                                      _origin);

    if (_cursor < 0)
    {
      SlideOut ((-_cursor + START_MARGING) * _time_scale,
                TRUE);
      _cursor = START_MARGING;
    }
    else if (_cursor > HOURS_MONITORED*G_TIME_SPAN_HOUR - START_MARGING)
    {
      SlideOut ((HOURS_MONITORED*G_TIME_SPAN_HOUR - START_MARGING - _cursor) * _time_scale,
               TRUE);
      _cursor = HOURS_MONITORED*G_TIME_SPAN_HOUR - START_MARGING;
    }

    Redraw ();

    if (_listener)
    {
      _listener->OnTimelineCursorMoved   ();
      _listener->OnTimelineCursorChanged ();
    }
  }

  // --------------------------------------------------------------------------------
  void Timeline::TranslateCursor (gdouble delta)
  {
    if (delta != 0.0)
    {
      // Translation
      {
        GooCanvasBounds bounds;

        goo_canvas_item_get_bounds (_goo_cursor,
                                    &bounds);
        if (delta < 0.0)
        {
          delta = MAX (delta, -bounds.x1);
        }
        else
        {
          delta = MIN (delta, _allocation.width - (bounds.x2 * 1.04));
        }

        goo_canvas_item_translate (_goo_cursor,
                                   delta,
                                   0.0);
        goo_canvas_item_translate (_goo_cursor_time,
                                   delta,
                                   0.0);
      }

      // Update the cursor attribute
      {
        GooCanvasBounds bounds;

        goo_canvas_item_get_bounds (_goo_cursor,
                                    &bounds);
        _cursor = bounds.x1 / _time_scale;
      }

      RefreshCursorTime ();

      if (_listener)
      {
        _listener->OnTimelineCursorMoved ();
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Timeline::RefreshCursorTime ()
  {
    GDateTime *time = RetreiveCursorTime ();
    gchar     *text;

#ifdef DEBUG
    text = g_strdup_printf (" <span weight=\"bold\" background=\"black\" foreground=\"white\"> %02d:%02d:%f </span>",
                            g_date_time_get_hour    (time),
                            g_date_time_get_minute  (time),
                            g_date_time_get_seconds (time));
#else
    text = g_strdup_printf (" <span weight=\"bold\" background=\"black\" foreground=\"white\"> %02d:%02d </span>",
                            g_date_time_get_hour    (time),
                            g_date_time_get_minute  (time));
#endif


    g_object_set (G_OBJECT (_goo_cursor_time),
                  "text", text,
                  NULL);

    g_free (text);
    g_date_time_unref (time);
  }

  // --------------------------------------------------------------------------------
  gboolean Timeline::OnButtonRelease (GooCanvasItem  *item,
                                      GooCanvasItem  *target,
                                      GdkEventButton *event,
                                      Timeline       *tl)
  {
    __gcc_extension__ g_signal_handlers_disconnect_by_func (item,
                                                            (void *) OnButtonRelease,
                                                            tl);
    __gcc_extension__ g_signal_handlers_disconnect_by_func (item,
                                                            (void *) OnMotion,
                                                            tl);

    {
      GTimeSpan round = tl->GetTimeRectification (tl->_cursor);

      tl->TranslateCursor (round * tl->_time_scale);
    }

    tl->ResetCursor ();

    if (tl->_listener)
    {
      tl->_listener->OnTimelineCursorChanged ();
    }

    return TRUE;
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_canvas_scrolled_window_size_allocate (GtkWidget    *widget,
                                                                           GdkRectangle *allocation,
                                                                           Object       *object)
  {
    Timeline *timeline = dynamic_cast <Timeline *> (object);

    timeline->Redraw ();
  }
}
