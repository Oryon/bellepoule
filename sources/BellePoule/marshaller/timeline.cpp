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
#include "timeslot.hpp"
#include "job.hpp"
#include "timeline.hpp"

// --------------------------------------------------------------------------------
Timeline::Timeline (Listener *listener)
  : CanvasModule ("timeline.glade",
                  "canvas_scrolled_window")
{
  _batch_list     = NULL;
  _redraw_timeout = 0;
  _listener       = listener;

  {
    GDateTime *now = g_date_time_new_now_local ();

    _origin = g_date_time_add_full (now,
                                    0,
                                    0,
                                    0,
                                    0,
                                    -g_date_time_get_minute (now) - START_MARGING/G_TIME_SPAN_MINUTE,
                                    -g_date_time_get_second (now));

    _cursor = g_date_time_difference (now,
                                      _origin);
    _cursor += 5 * G_TIME_SPAN_MINUTE;

    g_date_time_unref (now);
  }
}

// --------------------------------------------------------------------------------
Timeline::~Timeline ()
{
  if (_redraw_timeout > 0)
  {
    g_source_remove (_redraw_timeout);
  }

  g_list_free       (_batch_list);
  g_date_time_unref (_origin);
}

// --------------------------------------------------------------------------------
void Timeline::OnPlugged ()
{
  CanvasModule::OnPlugged ();

  g_object_set (G_OBJECT (GetCanvas ()),
                "bounds-padding",     0.0,
                "bounds-from-origin", TRUE,
                NULL);
}

// --------------------------------------------------------------------------------
void Timeline::AddBatch (Batch *batch)
{
  _batch_list = g_list_append (_batch_list,
                               batch);
}

// --------------------------------------------------------------------------------
void Timeline::RemoveBatch (Batch *batch)
{
  GList *node = g_list_find (_batch_list,
                             batch);

  if (node)
  {
    _batch_list = g_list_delete_link (_batch_list,
                                      node);
  }
}

// --------------------------------------------------------------------------------
void Timeline::DrawTimes ()
{
  for (guint i = 0; i <= HOURS_MONITORED; i++)
  {
    guint  h    = (g_date_time_get_hour (_origin) + i+1) % 24;
    gchar *font = g_strdup_printf (BP_FONT "%dpx", 10);
    gchar *time = g_strdup_printf ("<span background=\"white\">%02d:00</span>", h);

    font = g_strdup_printf (BP_FONT "%dpx", 10);
    time = g_strdup_printf ("<span background=\"white\">%02d:00</span>", h);

    goo_canvas_rect_new (GetRootItem (),
                         (i*G_TIME_SPAN_HOUR + START_MARGING) * _time_scale,
                         0.7 * _batch_scale,
                         0.5,
                         0.3 * _batch_scale,
                         "fill-color",     "black",
                         "stroke-pattern", NULL,
                         NULL);
    goo_canvas_text_new (GetRootItem (),
                         time,
                         (i*G_TIME_SPAN_HOUR + START_MARGING) *_time_scale,
                         _batch_scale,
                         -1.0,
                         GTK_ANCHOR_S,
                         "fill-color", "grey",
                         "font",       font,
                         "use-markup", TRUE,
                         NULL);
    g_free (time);
    g_free (font);
  }
}

// --------------------------------------------------------------------------------
void Timeline::DrawCursors ()
{
  {
    GDateTime *now = g_date_time_new_now_local ();
    GTimeSpan  x   = g_date_time_difference (now, _origin);

    goo_canvas_rect_new (GetRootItem (),
                         0.0,
                         0.0,
                         x * _time_scale,
                         _batch_scale,
                         "fill-color-rgba", 0x0000000F,
                         "stroke-pattern", NULL,
                         NULL);
    g_date_time_unref (now);
  }

  {
    GooCanvasItem *item = goo_canvas_rect_new (GetRootItem (),
                                               _cursor * _time_scale,
                                               0.0,
                                               2.0,
                                               _batch_scale,
                                               "fill-color",     "black",
                                               "stroke-pattern", NULL,
                                               NULL);

    g_signal_connect (item, "button_press_event",
                      G_CALLBACK (OnButtonPress), this);
  }
}

// --------------------------------------------------------------------------------
void Timeline::DrawSlots ()
{
  GList *current_batch = _batch_list;

  for (guint i = 0; current_batch != NULL; i++)
  {
    Batch *batch       = (Batch *) current_batch->data;
    gchar *color       = gdk_color_to_string (batch->GetColor ());
    GList *current_job = batch->GetScheduledJobs ();

    while (current_job != NULL)
    {
      Job       *job   = (Job *) current_job->data;
      TimeSlot  *slot  = job->GetTimslot ();
      GDateTime *start = slot->GetStartTime ();
      gdouble    x     = g_date_time_difference (start, _origin) * _time_scale;
      gdouble    w     = slot->GetDuration () * _time_scale;

      goo_canvas_rect_new (GetRootItem (),
                           x,
                           i*_batch_scale/10.0,
                           w,
                           _batch_scale/10.0,
                           "fill-color",     color,
                           "stroke-pattern", NULL,
                           NULL);

      current_job = g_list_next (current_job);
    }

    g_free (color);

    current_batch = g_list_next (current_batch);
  }
}

// --------------------------------------------------------------------------------
gboolean Timeline::RedrawCbk (Timeline *tl)
{
  return tl->Redraw ();
}

// --------------------------------------------------------------------------------
gboolean Timeline::Redraw ()
{
  Wipe ();

  {
    gtk_widget_get_allocation (GetRootWidget (),
                               &_allocation);

    _batch_scale = (gdouble)_allocation.height;
    _time_scale  = (gdouble)_allocation.width
      / (HOURS_MONITORED*G_TIME_SPAN_HOUR + START_MARGING);
  }

  DrawSlots   ();
  DrawTimes   ();
  DrawCursors ();

  if (_redraw_timeout > 0)
  {
    g_source_remove (_redraw_timeout);
  }

  _redraw_timeout = g_timeout_add_seconds (60,
                                           (GSourceFunc) RedrawCbk,
                                           this);

  //return G_SOURCE_CONTINUE;
  return TRUE;
}

// --------------------------------------------------------------------------------
gboolean Timeline::OnButtonPress (GooCanvasItem  *item,
                                  GooCanvasItem  *target,
                                  GdkEventButton *event,
                                  Timeline       *tl)
{
  tl->_cursor = event->x_root / tl->_time_scale;

  g_signal_connect (G_OBJECT (item), "motion-notify-event",
                    G_CALLBACK (OnMotion), tl);
  g_signal_connect (G_OBJECT (item), "button_release_event",
                    G_CALLBACK (OnButtonRelease), tl);
  return TRUE;
}

// --------------------------------------------------------------------------------
GDateTime *Timeline::RetreiveCursorTime ()
{
  return g_date_time_add (_origin,
                          _cursor);
}

// --------------------------------------------------------------------------------
gboolean Timeline::OnMotion (GooCanvasItem  *item,
                             GooCanvasItem  *target_item,
                             GdkEventMotion *event,
                             Timeline       *tl)
{
  if (item)
  {
    gdouble new_x = event->x_root;

    if (new_x < 2.0)
    {
      new_x = 2.0;
    }
    if (new_x > tl->_allocation.width - 2.0)
    {
      new_x = tl->_allocation.width - 2.0;
    }

    goo_canvas_item_translate (item,
                               new_x - tl->_cursor * tl->_time_scale,
                               0.0);

    tl->_cursor = new_x / tl->_time_scale;

    if (tl->_listener)
    {
      tl->_listener->OnTimelineCursorMoved ();
    }
    return TRUE;
  }

  return FALSE;
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
  return TRUE;
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_canvas_scrolled_window_size_allocate (GtkWidget    *widget,
                                                                         GdkRectangle *allocation,
                                                                         Object       *object)
{
  Timeline *timeline = dynamic_cast <Timeline *> (object);

  Timeline::RedrawCbk (timeline);
}
