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

#include <math.h>

#include "util/global.hpp"
#include "util/module.hpp"
#include "util/canvas.hpp"
#include "actors/referee.hpp"
#include "job.hpp"

#include "batch.hpp"
#include "piste.hpp"

const gdouble Piste::_W          = 160.0;
const gdouble Piste::_H          = 20.0;
const gdouble Piste::_BORDER_W   = 0.5;
const gdouble Piste::_RESOLUTION = 10.0;

// --------------------------------------------------------------------------------
Piste::Piste (GooCanvasItem *parent,
              Module        *container)
  : DropZone (container)
{
  _horizontal       = TRUE;
  _listener         = NULL;
  _timeslots        = NULL;
  _current_timeslot = NULL;

  _root_item = goo_canvas_group_new (parent,
                                     NULL);

  {
    _drop_rect = goo_canvas_rect_new (_root_item,
                                      0.0, 0.0,
                                      _W, _H,
                                      //"line-width",   _BORDER_W,
                                      //"stroke-color", "black",
                                      NULL);
    MonitorEvent (_drop_rect);
  }

  {
    _progress_item = goo_canvas_rect_new (_root_item,
                                          _BORDER_W,
                                          _H - (_H/10.0),
                                          _W/2.0,
                                          (_H/10.0)-_BORDER_W,
                                          "fill-color", "#fe7418",
                                          "line-width", 0.0,
                                          NULL);
    MonitorEvent (_progress_item);
  }

  {
    _id_item = goo_canvas_text_new (_root_item,
                                    "",
                                    1.0, 1.0,
                                    -1.0,
                                    GTK_ANCHOR_NW,
                                    "fill-color", "black",
                                    "font", BP_FONT "bold 10px",
                                    NULL);
    MonitorEvent (_id_item);
  }

  {
    _title_item = goo_canvas_text_new (_root_item,
                                       "",
                                       _W/2.0, 1.0,
                                       -1.0,
                                       GTK_ANCHOR_NORTH,
                                       "fill-color", "black",
                                       "font", BP_FONT "8px",
                                       NULL);
    MonitorEvent (_title_item);
  }

  {
    _match_item = goo_canvas_text_new (_root_item,
                                       "",
                                       _W/2.0, 9.0,
                                       -1.0,
                                       GTK_ANCHOR_NORTH,
                                       "fill-color", "black",
                                       "font", BP_FONT "bold 8px",
                                       NULL);
    MonitorEvent (_match_item);
  }

  {
    gchar     *icon_file = g_build_filename (Global::_share_dir, "resources", "glade", "images", "referee.png", NULL);
    GdkPixbuf *pixbuf    = container->GetPixbuf (icon_file);

    _status_item = goo_canvas_image_new (_root_item,
                                         pixbuf,
                                         (_W - 10.0) * 2.0,
                                         5.0,
                                         NULL);
    MonitorEvent (_status_item);

    goo_canvas_item_scale (_status_item,
                           0.5,
                           0.5);
    g_object_set (G_OBJECT (_status_item),
                  "visibility", GOO_CANVAS_ITEM_INVISIBLE,
                  NULL);

    g_object_unref (pixbuf);
    g_free (icon_file);
  }

  goo_canvas_item_translate (_root_item,
                             _RESOLUTION,
                             _RESOLUTION);

  SetId (1);

  _focus_color = g_strdup ("grey");
  _color       = g_strdup ("lightgrey");
  SetColor (_color);
}

// --------------------------------------------------------------------------------
Piste::~Piste ()
{
  g_list_free_full (_timeslots,
                    (GDestroyNotify) TryToRelease);

  goo_canvas_item_remove (_root_item);
  g_free (_color);
  g_free (_focus_color);
}

// --------------------------------------------------------------------------------
void Piste::SetListener (Listener *listener)
{
  _listener = listener;
}

// --------------------------------------------------------------------------------
void Piste::ScheduleJob (Job *job)
{
  TimeSlot *timeslot = GetFreeTimeslot ();

  timeslot->AddJob (job);
}

// --------------------------------------------------------------------------------
void Piste::SetCurrentTimeslot (GDateTime *start_time)
{
  GList *current = _timeslots;

  _current_timeslot = NULL;
  while (current)
  {
    TimeSlot *timeslot = (TimeSlot *) current->data;

    if (g_date_time_compare (timeslot->GetStartTime (),
                             start_time) <= 0)
    {
      GDateTime *end_time = g_date_time_add (timeslot->GetStartTime (),
                                             timeslot->GetDuration  ());

      if (g_date_time_compare (start_time,
                               end_time) <= 0)
      {
        _current_timeslot = timeslot;
        g_free (end_time);
        break;
      }
      g_free (end_time);
    }

    current = g_list_next (current);
  }

  if (_current_timeslot == NULL)
  {
    _current_timeslot = new TimeSlot (this,
                                      start_time);
  }
}

// --------------------------------------------------------------------------------
TimeSlot *Piste::GetFreeTimeslot ()
{
  TimeSlot  *free_timeslot;
  GList     *last_node     = g_list_last (_timeslots);
  GDateTime *start_time;

  if (last_node)
  {
    TimeSlot *last = (TimeSlot *) last_node->data;

    start_time = g_date_time_add (last->GetStartTime (),
                                  last->GetDuration ());
  }
  else
  {
    start_time = g_date_time_new_now_local ();
  }

  free_timeslot = new TimeSlot (this,
                                start_time);
  _timeslots = g_list_append (_timeslots,
                               free_timeslot);

  return free_timeslot;
}

// --------------------------------------------------------------------------------
void Piste::OnTimeSlotUpdated (TimeSlot *timeslot)
{
  if (timeslot == _timeslots->data)
  {
    GString     *match_name = g_string_new ("");
    GList       *current    = timeslot->GetJobList ();
    const gchar *last_name  = NULL;

    {
      g_free (_color);
      _color = g_strdup ("lightgrey");
      SetColor (_color);

      g_object_set (G_OBJECT (_title_item),
                    "text", "",
                    NULL);
    }

    if (timeslot->GetRefereeList ())
    {
      g_object_set (G_OBJECT (_status_item),
                    "visibility", GOO_CANVAS_ITEM_VISIBLE,
                    NULL);
    }
    else
    {
      g_object_set (G_OBJECT (_status_item),
                    "visibility", GOO_CANVAS_ITEM_INVISIBLE,
                    NULL);
    }

    for (guint i = 0; current != NULL; i++)
    {
      Job *job = (Job *) current->data;

      if (i == 0)
      {
        {
          g_free (_color);
          _color = gdk_color_to_string (job->GetGdkColor ());

          SetColor (_color);
        }

        {
          Batch *batch = job->GetBatch ();

          g_object_set (G_OBJECT (_title_item),
                        "text", batch->GetName (),
                        NULL);
        }

        g_string_append (match_name, job->GetName ());
      }
      else
      {
        if (i == 1)
        {
          g_string_append (match_name, " ... ");
        }

        last_name = job->GetName ();
      }

      current = g_list_next (current);
    }

    if (last_name)
    {
      g_string_append (match_name, last_name);
    }

    g_object_set (G_OBJECT (_match_item),
                  "text", match_name->str,
                  NULL);
    g_string_free (match_name, TRUE);
  }

  if (timeslot->GetJobList () == NULL)
  {
    GList *node = g_list_find (_timeslots,
                               timeslot);

    if (node)
    {
      _timeslots = g_list_delete_link (_timeslots,
                                        node);
    }
    timeslot->Release ();
  }

}

// --------------------------------------------------------------------------------
void Piste::AddReferee (Referee *referee)
{
  TimeSlot *timeslot = GetFreeTimeslot ();

  timeslot->AddReferee (referee);
}

// --------------------------------------------------------------------------------
void Piste::RemoveBatch (Batch *batch)
{
  GList *timeslot_list    = g_list_copy (_timeslots);
  GList *current_timeslot = timeslot_list;

  while (current_timeslot)
  {
    TimeSlot *timeslot = (TimeSlot *) current_timeslot->data;
    GList    *job_list = g_list_copy (timeslot->GetJobList ());

    {
      GList *current_job = job_list;

      while (current_job)
      {
        Job *job = (Job *) current_job->data;

        if (job->GetBatch () == batch)
        {
          timeslot->RemoveJob (job);
        }

        current_job = g_list_next (current_job);
      }

      g_list_free (job_list);
    }

    current_timeslot = g_list_next (current_timeslot);
  }

  g_list_free (timeslot_list);
}

// --------------------------------------------------------------------------------
void Piste::SetColor (const gchar *color)
{
  g_object_set (_drop_rect,
                "fill-color", color,
                NULL);
}

// --------------------------------------------------------------------------------
void Piste::Focus ()
{
  DropZone::Focus ();

  SetColor ("grey");
}

// --------------------------------------------------------------------------------
void Piste::Unfocus ()
{
  DropZone::Unfocus ();

  SetColor (_color);
}

// --------------------------------------------------------------------------------
void Piste::Translate (gdouble tx,
                       gdouble ty)
{
  if (_horizontal)
  {
    goo_canvas_item_translate (_root_item,
                               tx,
                               ty);
  }
  else
  {
    goo_canvas_item_translate (_root_item,
                               ty,
                               -tx);
  }
}

// --------------------------------------------------------------------------------
void Piste::Rotate ()
{
  if (_horizontal)
  {
    goo_canvas_item_rotate (_root_item,
                            90.0,
                            0.0,
                            0.0);
    _horizontal = FALSE;
  }
  else
  {
    goo_canvas_item_rotate (_root_item,
                            -90.0,
                            0.0,
                            0.0);
    _horizontal = TRUE;
  }
}

// --------------------------------------------------------------------------------
guint Piste::GetId ()
{
  return _id;
}

// --------------------------------------------------------------------------------
void Piste::SetId (guint id)
{
  _id = id;

  {
    gchar *name = g_strdup_printf ("%d", _id);

    g_object_set (_id_item,
                  "text", name,
                  NULL);

    g_free (name);
  }
}

// --------------------------------------------------------------------------------
gboolean Piste::OnButtonPress (GooCanvasItem  *item,
                               GooCanvasItem  *target,
                               GdkEventButton *event,
                               Piste          *piste)
{
  if (event->button == 1)
  {
    piste->_listener->OnPisteButtonEvent (piste,
                                          event);

    return TRUE;
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean Piste::OnButtonRelease (GooCanvasItem  *item,
                                 GooCanvasItem  *target,
                                 GdkEventButton *event,
                                 Piste          *piste)
{
  if (event->button == 1)
  {
    piste->_listener->OnPisteButtonEvent (piste,
                                          event);

    return TRUE;
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
void Piste::Disable ()
{
  GList *current = _timeslots;

  while (current)
  {
    TimeSlot *timeslot = (TimeSlot *) current->data;

    timeslot->Cancel ();

    current = g_list_next (current);
  }
}

// --------------------------------------------------------------------------------
void Piste::Select ()
{
  GooCanvasLineDash *dash = goo_canvas_line_dash_new (2, 10.0, 1.0);

  g_object_set (_drop_rect,
                "line-dash", dash,
                NULL);
  goo_canvas_line_dash_unref (dash);
}

// --------------------------------------------------------------------------------
void Piste::UnSelect ()
{
  GooCanvasLineDash *dash = goo_canvas_line_dash_new (0);

  g_object_set (_drop_rect,
                "line-dash", dash,
                NULL);
  goo_canvas_line_dash_unref (dash);
}

// --------------------------------------------------------------------------------
gboolean Piste::OnMotionNotify (GooCanvasItem  *item,
                                GooCanvasItem  *target,
                                GdkEventMotion *event,
                                Piste          *piste)
{
  piste->_listener->OnPisteMotionEvent (piste,
                                        event);

  return TRUE;
}

// --------------------------------------------------------------------------------
void Piste::GetHorizontalCoord (gdouble *x,
                                gdouble *y)
{
  if (_horizontal == FALSE)
  {
    gdouble swap = *x;

    *x = -(*y);
    *y = swap;
  }
}

// --------------------------------------------------------------------------------
gdouble Piste::GetGridAdjustment (gdouble coordinate)
{
  gdouble modulo = fmod (coordinate, _RESOLUTION);

  if (modulo > _RESOLUTION/2.0)
  {
    return _RESOLUTION - modulo;
  }
  else
  {
    return -modulo;
  }
}

// --------------------------------------------------------------------------------
void Piste::AlignOnGrid ()
{
  GooCanvasBounds bounds;

  goo_canvas_item_get_bounds (_root_item,
                              &bounds);

  goo_canvas_item_translate (_root_item,
                             GetGridAdjustment (bounds.x1),
                             GetGridAdjustment (bounds.y1));
}

// --------------------------------------------------------------------------------
void Piste::AnchorTo (Piste *piste)
{
  GooCanvasBounds to_bounds;
  GooCanvasBounds bounds;

  goo_canvas_item_get_bounds (piste->_root_item,
                              &to_bounds);
  goo_canvas_item_get_bounds (_root_item,
                              &bounds);

  goo_canvas_item_translate (_root_item,
                             to_bounds.x1 - bounds.x1,
                             (to_bounds.y1 + _H*1.5) - bounds.y1);
}

// --------------------------------------------------------------------------------
void Piste::MonitorEvent (GooCanvasItem *item)
{
  g_signal_connect (item,
                    "motion_notify_event",
                    G_CALLBACK (OnMotionNotify),
                    this);

  g_signal_connect (item,
                    "button_press_event",
                    G_CALLBACK (OnButtonPress),
                    this);

  g_signal_connect (item,
                    "button_release_event",
                    G_CALLBACK (OnButtonRelease),
                    this);
}

// --------------------------------------------------------------------------------
gint Piste::CompareJob (Job *a,
                        Job *b)
{
  return strcmp (a->GetName (), b->GetName ());
}

// --------------------------------------------------------------------------------
gint Piste::CompareAvailbility (Piste *a,
                                Piste *b)
{
  GList *timeslot_a = g_list_last (a->_timeslots);
  GList *timeslot_b = g_list_last (b->_timeslots);

  if (timeslot_a && timeslot_b)
  {
    return TimeSlot::CompareAvailbility ((TimeSlot *) timeslot_a->data,
                                         (TimeSlot *) timeslot_b->data);
  }
  else if (timeslot_a == timeslot_b)
  {
    return a->_id - b->_id;
  }
  else if (timeslot_b == NULL)
  {
    return 1;
  }
  else
  {
    return -1;
  }
}
