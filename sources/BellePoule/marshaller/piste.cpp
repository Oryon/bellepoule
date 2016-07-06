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
#include "enlisted_referee.hpp"
#include "job.hpp"

#include "batch.hpp"
#include "piste.hpp"

namespace Marshaller
{
  const gdouble Piste::_W          = 160.0;
  const gdouble Piste::_H          = 20.0;
  const gdouble Piste::_BORDER_W   = 0.5;
  const gdouble Piste::_RESOLUTION = 10.0;

  // --------------------------------------------------------------------------------
  Piste::Piste (GooCanvasItem *parent,
                Module        *container)
    : DropZone (container)
  {
    _horizontal = TRUE;
    _listener   = NULL;
    _timeslots  = NULL;

    _display_time = g_date_time_new_now_local ();

    _root_item = goo_canvas_group_new (parent,
                                       NULL);

    // Background
    {
      _drop_rect = goo_canvas_rect_new (_root_item,
                                        0.0, 0.0,
                                        _W, _H,
                                        "line-width",   _BORDER_W,
                                        //"stroke-color", "green",
                                        NULL);
      MonitorEvent (_drop_rect);
    }

    // Progression
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

    // #
    {
      _id_item = goo_canvas_text_new (_root_item,
                                      "",
                                      1.0, 1.0,
                                      -1.0,
                                      GTK_ANCHOR_NW,
                                      "font", BP_FONT "bold 10px",
                                      NULL);
      MonitorEvent (_id_item);
    }

    // Title
    {
      _title_item = goo_canvas_text_new (_root_item,
                                         "",
                                         _W/2.0, 1.0,
                                         -1.0,
                                         GTK_ANCHOR_NORTH,
                                         "font",       BP_FONT "8px",
                                         "use-markup", TRUE,
                                         NULL);
      MonitorEvent (_title_item);
    }

    // Referee
    {
      _referee_table = goo_canvas_table_new (_root_item,
                                             "x", (gdouble) 10.0,
                                             "y", _H*0.5,
                                             "column-spacing", (gdouble) 4.0,
                                             NULL);

      {
        gchar         *icon_file = g_build_filename (Global::_share_dir, "resources", "glade", "images", "referee.png", NULL);
        GdkPixbuf     *pixbuf    = container->GetPixbuf (icon_file);
        GooCanvasItem *icon      = Canvas::PutPixbufInTable (_referee_table,
                                                             pixbuf,
                                                             0, 1);

        goo_canvas_item_scale (icon,
                               0.5,
                               0.5);

        g_object_unref (pixbuf);
        g_free (icon_file);
      }

      {
        _referee_name = Canvas::PutTextInTable (_referee_table,
                                                "",
                                                0, 2);
        g_object_set (G_OBJECT (_referee_name),
                      "font", BP_FONT "bold 6px",
                      NULL);
        Canvas::SetTableItemAttribute (_referee_name, "y-align", 1.0);
      }

      MonitorEvent (_referee_table);
    }

    goo_canvas_item_translate (_root_item,
                               _RESOLUTION,
                               _RESOLUTION);

    g_object_set (G_OBJECT (_referee_table),
                  "visibility", GOO_CANVAS_ITEM_INVISIBLE,
                  NULL);

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

    if (_display_time)
    {
      g_date_time_unref (_display_time);
    }
  }

  // --------------------------------------------------------------------------------
  void Piste::SetListener (Listener *listener)
  {
    _listener = listener;
  }

  // --------------------------------------------------------------------------------
  TimeSlot *Piste::GetTimeslotAt (GDateTime *start_time)
  {
    GList *current = _timeslots;

    while (current)
    {
      TimeSlot *timeslot = (TimeSlot *) current->data;

      if (TimeIsInTimeslot (start_time,
                            timeslot))
      {
        return timeslot;
      }

      current = g_list_next (current);
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  void Piste::DisplayAtTime (GDateTime *time)
  {
    if (_display_time)
    {
      g_date_time_unref (_display_time);
    }
    _display_time = g_date_time_add (time, 0);

    CleanDisplay      ();
    OnSlotUpdated (GetTimeslotAt (time));
  }

  // --------------------------------------------------------------------------------
  gboolean Piste::TimeIsInTimeslot (GDateTime *time,
                                    TimeSlot  *timeslot)
  {
    if (timeslot && (g_date_time_compare (timeslot->GetStartTime (), time) <= 0))
    {
      GDateTime *end_time = g_date_time_add (timeslot->GetStartTime (),
                                             timeslot->GetDuration  ());

      if (g_date_time_compare (time,
                               end_time) <= 0)
      {
        g_date_time_unref (end_time);
        return TRUE;
      }
      g_date_time_unref (end_time);
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  GList *Piste::GetFreeSlots (GDateTime *from,
                              GTimeSpan  duration)
  {
    GList    *slots        = NULL;
    GList    *next         = NULL;
    TimeSlot *current_slot = NULL;

    {
      GList *current = _timeslots;

      while (current)
      {
        current_slot = (TimeSlot *) current->data;
        next         = g_list_next (current);

        if (next)
        {
          TimeSlot  *next_slot = (TimeSlot *) next->data;
          GTimeSpan  interval  = current_slot->GetInterval (next_slot);

          if (interval > duration)
          {
            break;
          }
        }
        else
        {
          break;
        }

        current = next;
      }
    }

    {
      GDateTime *start_time = NULL;

      if (current_slot)
      {
        start_time = g_date_time_add (current_slot->GetStartTime (),
                                      current_slot->GetDuration ());
      }
      else
      {
        GDateTime *now = g_date_time_new_now_local ();
        GDateTime *tmp;

        tmp = g_date_time_add_minutes (now, 15);
        g_date_time_unref (now);
        now = tmp;

        // Round minutes
        {
          gint minutes = g_date_time_get_minute (now);
          gint crumbs  = minutes % 15;

          if (crumbs > 15/2)
          {
            tmp = g_date_time_add_minutes (now, 15 - crumbs);
            g_date_time_unref (now);
            now = tmp;
          }
          else
          {
            tmp = g_date_time_add_minutes (now, -crumbs);
            g_date_time_unref (now);
            now = tmp;
          }
        }

        start_time = g_date_time_new_local (g_date_time_get_year         (now),
                                            g_date_time_get_month        (now),
                                            g_date_time_get_day_of_month (now),
                                            g_date_time_get_hour         (now),
                                            g_date_time_get_minute       (now),
                                            0);
        g_date_time_unref (now);
      }

      {
        TimeSlot *free_timeslot;

        free_timeslot = new TimeSlot (this,
                                      start_time,
                                      duration);

        slots = g_list_append (slots,
                               free_timeslot);
      }
    }

    return slots;
  }

  // --------------------------------------------------------------------------------
  void Piste::CleanDisplay ()
  {
    g_free (_color);
    _color = g_strdup ("lightgrey");
    SetColor (_color);

    g_object_set (G_OBJECT (_title_item),
                  "text", "",
                  NULL);

    g_object_set (G_OBJECT (_referee_table),
                  "visibility", GOO_CANVAS_ITEM_INVISIBLE,
                  NULL);
  }

  // --------------------------------------------------------------------------------
  void Piste::OnSlotUpdated (TimeSlot *timeslot)
  {
    if (TimeIsInTimeslot (_display_time, timeslot))
    {
      GString     *match_name   = g_string_new ("");
      const gchar *last_name    = NULL;
      GList       *referee_list = timeslot->GetRefereeList ();

      CleanDisplay ();

      if (referee_list)
      {
        EnlistedReferee *referee = (EnlistedReferee *) referee_list->data;
        gchar           *name    = referee->GetName ();

        g_object_set (G_OBJECT (_referee_table),
                      "visibility", GOO_CANVAS_ITEM_VISIBLE,
                      NULL);

        g_object_set (G_OBJECT (_referee_name),
                      "text", name,
                      NULL);
        g_free (name);
      }
      else
      {
        g_object_set (G_OBJECT (_referee_table),
                      "visibility", GOO_CANVAS_ITEM_INVISIBLE,
                      NULL);
      }

      {
        GList *current = timeslot->GetJobList ();

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

              g_string_append (match_name, batch->GetName ());
              g_string_append (match_name, " - <b>");
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
      }

      if (last_name)
      {
        g_string_append (match_name, last_name);
      }

      g_string_append (match_name, "</b>");

      g_object_set (G_OBJECT (_title_item),
                    "text", match_name->str,
                    NULL);
      g_string_free (match_name, TRUE);
    }

    if (timeslot && (timeslot->GetJobList () == NULL))
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
  void Piste::OnSlotLocked (TimeSlot *timeslot)
  {
    _timeslots = g_list_insert_sorted (_timeslots,
                                       timeslot,
                                       (GCompareFunc) TimeSlot::CompareAvailbility);
  }

  // --------------------------------------------------------------------------------
  void Piste::AddReferee (EnlistedReferee *referee)
  {
    TimeSlot *timeslot = GetTimeslotAt (_display_time);

    if (timeslot)
    {
      timeslot->AddReferee (referee);
    }
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
  void Piste::ConvertFromPisteSpace (gdouble *x,
                                     gdouble *y)
  {
    goo_canvas_convert_from_item_space  (goo_canvas_item_get_canvas (_root_item),
                                         _root_item,
                                         x,
                                         y);
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
  gboolean Piste::Overlaps (GooCanvasBounds *rectangle)
  {
    GooCanvasBounds bounds;

    goo_canvas_item_get_bounds (_root_item,
                                &bounds);

    return !(   rectangle->y2 < bounds.y1
             || rectangle->y1 > bounds.y2
             || rectangle->x2 < bounds.x1
             || rectangle->x1 > bounds.x2);
  }

  // --------------------------------------------------------------------------------
  void Piste::Select ()
  {
    g_object_set (_drop_rect,
                  "line-width", _BORDER_W*4.0,
                  NULL);
  }

  // --------------------------------------------------------------------------------
  void Piste::UnSelect ()
  {
    g_object_set (_drop_rect,
                  "line-width", _BORDER_W,
                  NULL);
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
    return g_strcmp0 (a->GetName (), b->GetName ());
  }
}
