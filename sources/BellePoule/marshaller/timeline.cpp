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
Timeline::Timeline ()
  : CanvasModule ("timeline.glade",
                  "canvas_scrolled_window")
{
  _batch_list = NULL;

  {
    GDateTime *now = g_date_time_new_now_local ();

    _origin = g_date_time_add_full (now,
                                    0,
                                    0,
                                    0,
                                    0,
                                    -g_date_time_get_minute (now),
                                    -g_date_time_get_second (now));
    g_date_time_unref (now);
  }
}

// --------------------------------------------------------------------------------
Timeline::~Timeline ()
{
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
  for (guint i = 1; i < HOURS_MONITORED; i++)
  {
    guint  h    = g_date_time_get_hour (_origin) + i;
    gchar *font = g_strdup_printf (BP_FONT "%dpx", 10);
    gchar *time = g_strdup_printf ("<span background=\"white\">%02d:00</span>", h);

    goo_canvas_rect_new (GetRootItem (),
                         i * G_TIME_SPAN_HOUR * _time_scale,
                         0.7 * _batch_scale,
                         0.5,
                         0.3 * _batch_scale,
                         "fill-color",     "black",
                         "stroke-pattern", NULL,
                         NULL);
    goo_canvas_text_new (GetRootItem (),
                         time,
                         i * G_TIME_SPAN_HOUR * _time_scale,
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
                                               (gdouble)_allocation.width / 4.0,
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
void Timeline::Redraw ()
{
  Wipe ();

  {
    gtk_widget_get_allocation (GetRootWidget (),
                               &_allocation);

    _time_scale  = (gdouble)_allocation.width / (G_TIME_SPAN_HOUR * HOURS_MONITORED);
    _batch_scale = (gdouble)_allocation.height;
  }

  DrawSlots   ();
  DrawTimes   ();
  DrawCursors ();
}

// --------------------------------------------------------------------------------
gboolean Timeline::OnButtonPress (GooCanvasItem  *item,
                                  GooCanvasItem  *target,
                                  GdkEventButton *event,
                                  Timeline       *timeline)
{
  timeline->_cursor_x = event->x_root;

  g_signal_connect (G_OBJECT (item), "motion-notify-event",
                    G_CALLBACK (OnMotion), timeline);
  g_signal_connect (G_OBJECT (item), "button_release_event",
                    G_CALLBACK (OnButtonRelease), timeline);
  return TRUE;
}

// --------------------------------------------------------------------------------
gboolean Timeline::OnMotion (GooCanvasItem  *item,
                             GooCanvasItem  *target_item,
                             GdkEventMotion *event,
                             Timeline       *timeline)
{
  if (item)
  {
    gdouble new_x = event->x_root;

    if (new_x < 2.0)
    {
      new_x = 2.0;
    }
    if (new_x > timeline->_allocation.width - 2.0)
    {
      new_x = timeline->_allocation.width - 2.0;
    }

    goo_canvas_item_translate (item,
                               new_x - timeline->_cursor_x,
                               0.0);

    timeline->_cursor_x = new_x;
    return TRUE;
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean Timeline::OnButtonRelease (GooCanvasItem  *item,
                                    GooCanvasItem  *target,
                                    GdkEventButton *event,
                                    Timeline       *timeline)
{
  __gcc_extension__ g_signal_handlers_disconnect_by_func (item,
                                                          (void *) OnButtonRelease,
                                                          timeline);
  __gcc_extension__ g_signal_handlers_disconnect_by_func (item,
                                                          (void *) OnMotion,
                                                          timeline);
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
