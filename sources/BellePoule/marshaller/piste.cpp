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
#include "util/attribute_desc.hpp"
#include "util/filter.hpp"
#include "enlisted_referee.hpp"
#include "job.hpp"
#include "job_board.hpp"
#include "batch.hpp"
#include "clock.hpp"
#include "slot.hpp"

#include "piste.hpp"

namespace Marshaller
{
  const gdouble Piste::_W          = 240.0;
  const gdouble Piste::_H          = 30.0;
  const gdouble Piste::_BORDER_W   = 0.75;
  const gdouble Piste::_RESOLUTION = 15.0;

  // --------------------------------------------------------------------------------
  Piste::Piste (Clock         *clock,
                GooCanvasItem *parent,
                Module        *container,
                Listener      *listener)
    : DropZone (container)
  {
    _button_press_time = 0;

    _blocked = FALSE;

    _horizontal = TRUE;
    _listener   = nullptr;
    _slots      = nullptr;

    _clock = clock;
    _clock->Retain ();

    _listener = listener;

    _job_board = new JobBoard ();

    _display_time = _clock->RetreiveNow ();

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
      _progress_rect = goo_canvas_rect_new (_root_item,
                                            _BORDER_W,
                                            _BORDER_W,
                                            0.0,
                                            _H-(2.0*_BORDER_W),
                                            "fill-color-rgba", 0xd3d3d360,
                                            "line-width",      0.0,
                                            "pointer-events", GOO_CANVAS_EVENTS_NONE,
                                            NULL);

      _progress_bar = goo_canvas_rect_new (_root_item,
                                           _BORDER_W,
                                           _BORDER_W,
                                           0.0,
                                           _H-(2.0*_BORDER_W),
                                           "fill-color-rgba", 0x00000060,
                                           "line-width",      0.0,
                                           "pointer-events", GOO_CANVAS_EVENTS_NONE,
                                           NULL);
    }

    // #
    {
      _id_item = goo_canvas_text_new (_root_item,
                                      "",
                                      1.5, 1.5,
                                      -1.0,
                                      GTK_ANCHOR_NW,
                                      "font", BP_FONT "bold 15px",
                                      "pointer-events", GOO_CANVAS_EVENTS_NONE,
                                      NULL);
    }

    // Title
    {
      _title_item = goo_canvas_text_new (_root_item,
                                         "",
                                         _W/2.0, 1.5,
                                         -1.0,
                                         GTK_ANCHOR_NORTH,
                                         "font",       BP_FONT "12px",
                                         "use-markup", TRUE,
                                         "pointer-events", GOO_CANVAS_EVENTS_NONE,
                                         NULL);
    }

    // Referee
    {
      _referee_table = goo_canvas_table_new (_root_item,
                                             "x", (gdouble) 15.0,
                                             "y", _H*0.5,
                                             "column-spacing", (gdouble) 4.0,
                                             "visibility",     GOO_CANVAS_ITEM_INVISIBLE,
                                             NULL);

      {
        gchar         *icon_file = g_build_filename (Global::_share_dir, "resources", "glade", "images", "referee.png", NULL);
        GdkPixbuf     *pixbuf    = container->GetPixbuf (icon_file);
        GooCanvasItem *icon      = Canvas::PutPixbufInTable (_referee_table,
                                                             pixbuf,
                                                             0, 1);

        goo_canvas_item_scale (icon,
                               0.8,
                               0.8);

        g_object_unref (pixbuf);
        g_free (icon_file);
      }

      {
        _referee_name = Canvas::PutTextInTable (_referee_table,
                                                "",
                                                0, 2);
        g_object_set (G_OBJECT (_referee_name),
                      "font", BP_FONT "bold 9px",
                      "pointer-events", GOO_CANVAS_EVENTS_NONE,
                      NULL);
        Canvas::SetTableItemAttribute (_referee_name, "y-align", 1.0);
      }
    }

    // Cone
    {
      gchar     *icon_file = g_build_filename (Global::_share_dir, "resources", "glade", "images", "VLC.png", NULL);
      GdkPixbuf *pixbuf    = gdk_pixbuf_new_from_file (icon_file, nullptr);

      _cone = goo_canvas_image_new (_root_item,
                                    pixbuf,
                                    0.0,
                                    0.0,
                                    "pointer-events", GOO_CANVAS_EVENTS_NONE,
                                    "visibility",     GOO_CANVAS_ITEM_INVISIBLE,
                                    NULL);
      g_object_unref (pixbuf);
      g_free (icon_file);

      Canvas::VAlign (_cone,
                      Canvas::Alignment::MIDDLE,
                      _root_item,
                      Canvas::Alignment::MIDDLE);
      Canvas::HAlign (_cone,
                      Canvas::Alignment::MIDDLE,
                      _root_item,
                      Canvas::Alignment::MIDDLE);
    }

    // Finish
    {
      gchar     *icon_file = g_build_filename (Global::_share_dir, "resources", "glade", "images", "finish.png", NULL);
      GdkPixbuf *pixbuf    = gdk_pixbuf_new_from_file (icon_file, nullptr);

      _finish = goo_canvas_image_new (_root_item,
                                      pixbuf,
                                      0.0,
                                      0.0,
                                      "pointer-events", GOO_CANVAS_EVENTS_NONE,
                                      "visibility",     GOO_CANVAS_ITEM_INVISIBLE,
                                      NULL);
      g_object_unref (pixbuf);
      g_free (icon_file);

      Canvas::VAlign (_finish,
                      Canvas::Alignment::MIDDLE,
                      _root_item,
                      Canvas::Alignment::END,
                      -15.0);
      Canvas::HAlign (_finish,
                      Canvas::Alignment::MIDDLE,
                      _root_item,
                      Canvas::Alignment::MIDDLE);
    }

    // Shutter
    {
      _shutter = goo_canvas_rect_new (_root_item,
                                      0.0, 0.0,
                                      _W, _H,
                                      "fill-color-rgba", 0xf3f3f3c0,
                                      "line-width",      0.0,
                                      "pointer-events", GOO_CANVAS_EVENTS_NONE,
                                      "visibility",     GOO_CANVAS_ITEM_INVISIBLE,
                                      NULL);
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
    FreeFullGList (Slot, _slots);

    goo_canvas_item_remove (_root_item);
    g_free (_color);
    g_free (_focus_color);

    if (_display_time)
    {
      g_date_time_unref (_display_time);
    }

    _job_board->Release ();
    _clock->Release ();

    _listener->OnPisteDirty ();
  }

  // --------------------------------------------------------------------------------
  gint Piste::CompareId (Piste *a,
                         Piste *b)
  {
    return a->_id >= b->_id;
  }

  // --------------------------------------------------------------------------------
  gboolean Piste::ReadJson (JsonReader *reader)
  {
    gint            x;
    gint            y;
    GooCanvasBounds bounds;

    goo_canvas_item_get_bounds (_root_item,
                                &bounds);

    json_reader_read_member (reader, "id");
    SetId (json_reader_get_int_value (reader));
    json_reader_end_member (reader);

    json_reader_read_member (reader, "blocked");
    Block (json_reader_get_int_value (reader));
    json_reader_end_member (reader);

    json_reader_read_member (reader, "x");
    x = json_reader_get_int_value (reader);
    json_reader_end_member (reader);

    json_reader_read_member (reader, "y");
    y = json_reader_get_int_value (reader);
    json_reader_end_member (reader);

    Translate (x - bounds.x1,
               y - bounds.y1);

    json_reader_read_member (reader, "rotation");
    if (json_reader_get_int_value (reader) == 90)
    {
      Rotate ();
      Translate (_H,
                 0.0);
    }
    json_reader_end_member (reader);

    return TRUE;
  }

  // --------------------------------------------------------------------------------
  void Piste::FeedJsonBuilder (JsonBuilder *builder)
  {
    GooCanvasBounds bounds;

    goo_canvas_item_get_bounds (_root_item,
                                &bounds);

    json_builder_begin_object (builder);

    json_builder_set_member_name (builder, "id");
    json_builder_add_int_value   (builder, _id);

    json_builder_set_member_name (builder, "blocked");
    json_builder_add_int_value   (builder, _blocked);

    json_builder_set_member_name (builder, "x");
    json_builder_add_int_value   (builder, bounds.x1);

    json_builder_set_member_name (builder, "y");
    json_builder_add_int_value   (builder, bounds.y1);

    json_builder_set_member_name (builder, "rotation");
    if (_horizontal)
    {
      json_builder_add_int_value (builder, 0);
    }
    else
    {
      json_builder_add_int_value (builder, 90);
    }

    json_builder_end_object (builder);
  }

  // --------------------------------------------------------------------------------
  GList *Piste::GetSlots ()
  {
    return _slots;
  }

  // --------------------------------------------------------------------------------
  Slot *Piste::GetSlotAt (GDateTime *time)
  {
    return Slot::GetSlotAt (time,
                            _slots);
  }

  // --------------------------------------------------------------------------------
  Slot *Piste::GetSlotAfter (Slot *slot)
  {
    GList *node = g_list_find (_slots,
                               slot);

    if (node)
    {
      node = g_list_next (node);

      if (node)
      {
        return (Slot *) node->data;
      }
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  void Piste::DisplayAtTime (GDateTime *time)
  {
    if (_display_time)
    {
      g_date_time_unref (_display_time);
    }
    _display_time = g_date_time_add (time, 0);

    OnSlotUpdated (GetSlotAt (time));
  }

  // --------------------------------------------------------------------------------
  Slot *Piste::CreateSlot (GDateTime *from,
                           GTimeSpan  duration)
  {
    return new Slot (this,
                     from,
                     nullptr,
                     duration);
  }

  // --------------------------------------------------------------------------------
  Slot *Piste::GetFreeSlot (GDateTime *from,
                            GTimeSpan  duration)
  {
    if (_blocked == FALSE)
    {
      return Slot::GetFreeSlot (this,
                                _slots,
                                from,
                                duration);
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  GList *Piste::GetFreeSlots (GDateTime *from,
                              GTimeSpan  duration)
  {
    if (_blocked == FALSE)
    {
      return Slot::GetFreeSlots (this,
                                 _slots,
                                 from,
                                 duration);
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  void Piste::CleanDisplay ()
  {
    g_free (_color);
    _color = g_strdup ("lightgrey");
    SetColor (_color);

    g_object_set (G_OBJECT (_progress_rect),
                  "width", 0.0,
                  NULL);

    g_object_set (G_OBJECT (_progress_bar),
                  "width", 0.0,
                  NULL);

    g_object_set (G_OBJECT (_title_item),
                  "text", "",
                  NULL);

    g_object_set (G_OBJECT (_referee_table),
                  "visibility", GOO_CANVAS_ITEM_INVISIBLE,
                  NULL);

    g_object_set (G_OBJECT (_shutter),
                  "visibility", GOO_CANVAS_ITEM_INVISIBLE,
                  NULL);

    g_object_set (G_OBJECT (_finish),
                  "visibility", GOO_CANVAS_ITEM_INVISIBLE,
                  NULL);
  }

  // --------------------------------------------------------------------------------
  void Piste::Blur ()
  {
    g_object_set (G_OBJECT (_shutter),
                  "visibility", GOO_CANVAS_ITEM_VISIBLE,
                  NULL);
  }

  // --------------------------------------------------------------------------------
  gboolean Piste::Blured ()
  {
    gboolean blured;

    g_object_get (G_OBJECT (_shutter),
                  "visibility", &blured,
                  NULL);

    return (blured == GOO_CANVAS_ITEM_VISIBLE);
  }

  // --------------------------------------------------------------------------------
  void Piste::Redraw ()
  {
    GDateTime *from = g_date_time_ref (_display_time);

    DisplayAtTime (from);
    g_date_time_unref (from);
  }

  // --------------------------------------------------------------------------------
  void Piste::OnSlotUpdated (Slot *slot)
  {
    CleanDisplay ();

    if (slot && slot->TimeIsInside (_display_time))
    {
      GString     *match_name   = g_string_new ("");
      const gchar *last_name    = nullptr;
      GList       *referee_list = slot->GetRefereeList ();

      if (slot->IsOver ())
      {
        g_object_set (G_OBJECT (_finish),
                      "visibility", GOO_CANVAS_ITEM_VISIBLE,
                      NULL);
      }
      else
      {
        g_object_set (G_OBJECT (_finish),
                      "visibility", GOO_CANVAS_ITEM_INVISIBLE,
                      NULL);
      }


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
        GList *current = slot->GetJobList ();

        for (guint i = 0; current != nullptr; i++)
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

      {
        GDateTime *start_time = slot->GetStartTime ();
        GTimeSpan  duration   = slot->GetDuration ();
        GDateTime *now        = _clock->RetreiveNow ();
        GTimeSpan  elapsed    = g_date_time_difference (now, start_time);
        gdouble    progress   = (elapsed * _W) / duration;

        if (elapsed > 0)
        {
          g_object_set (G_OBJECT (_progress_rect),
                        "width", MIN (progress, _W),
                        NULL);
          g_object_set (G_OBJECT (_progress_bar),
                        "x",     MIN (progress, _W),
                        "width", 0.7,
                        NULL);
        }
        else
        {
          g_object_set (G_OBJECT (_progress_rect),
                        "width", 0.0,
                        NULL);
          g_object_set (G_OBJECT (_progress_bar),
                        "width", 0.0,
                        NULL);
        }

        g_date_time_unref (now);
      }

      if (last_name)
      {
        g_string_append (match_name, last_name);
      }

      if (match_name->str[0] != '\0')
      {
        g_string_append (match_name, "</b>");
      }

      g_object_set (G_OBJECT (_title_item),
                    "text", match_name->str,
                    NULL);
      g_string_free (match_name, TRUE);
    }
  }

  // --------------------------------------------------------------------------------
  void Piste::OnSlotAssigned (Slot *slot)
  {
    _slots = g_list_insert_sorted (_slots,
                                   slot,
                                   (GCompareFunc) Slot::CompareAvailbility);
  }

  // --------------------------------------------------------------------------------
  void Piste::OnSlotRetracted (Slot *slot)
  {
    if (slot && (slot->GetJobList () == nullptr))
    {
      _slots = g_list_remove (_slots,
                              slot);

      slot->Release ();
    }
  }

  // --------------------------------------------------------------------------------
  void Piste::AddReferee (EnlistedReferee *referee)
  {
    Slot *slot = Slot::GetSlotAt (_display_time,
                                  _slots);

    if (slot)
    {
      slot->AddReferee (referee);
    }
  }

  // --------------------------------------------------------------------------------
  void Piste::RemoveBatch (Batch *batch)
  {
    GList *slot_list    = g_list_copy (_slots);
    GList *current_slot = slot_list;

    while (current_slot)
    {
      Slot  *slot     = (Slot *) current_slot->data;
      GList *job_list = g_list_copy (slot->GetJobList ());

      {
        GList *current_job = job_list;

        while (current_job)
        {
          Job *job = (Job *) current_job->data;

          if (job->GetBatch () == batch)
          {
            if (job->NeedRoadmap ())
            {
              slot->RemoveJob (job);
              job->ResetRoadMap ();
            }
          }

          current_job = g_list_next (current_job);
        }

        g_list_free (job_list);
      }

      current_slot = g_list_next (current_slot);
    }

    g_list_free (slot_list);
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

    _listener->OnPisteDirty ();
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

    _listener->OnPisteDirty ();
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
  void Piste::OnDoubleClick (Piste          *piste,
                             GdkEventButton *event)
  {
    GList *slots = piste->GetSlots ();

    if (slots)
    {
      _job_board->Clean ();

      while (slots)
      {
        Slot *slot = (Slot *) slots->data;

        _job_board->AddJobs (slot->GetJobList ());

        slots = g_list_next (slots);
      }

      _job_board->Display ();
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
      if (event->time == piste->_button_press_time)
      {
        piste->_listener->OnPisteButtonEvent (piste,
                                              nullptr);
        piste->OnDoubleClick (piste,
                              event);
      }
      else
      {
        piste->_listener->OnPisteButtonEvent (piste,
                                              event);
      }
      piste->_button_press_time = event->time;

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
  gboolean Piste::IsBlocked ()
  {
    return _blocked;
  }

  // --------------------------------------------------------------------------------
  void Piste::Block (gboolean block)
  {
    _blocked = block;

    if (_blocked)
    {
      g_object_set (G_OBJECT (_cone),
                    "visibility", GOO_CANVAS_ITEM_VISIBLE,
                    NULL);
    }
    else
    {
      g_object_set (G_OBJECT (_cone),
                    "visibility", GOO_CANVAS_ITEM_INVISIBLE,
                    NULL);
    }

    _listener->OnPisteDirty ();
  }

  // --------------------------------------------------------------------------------
  void Piste::Disable ()
  {
    GList *current = _slots;

    while (current)
    {
      Slot *slot = (Slot *) current->data;

      slot->Cancel ();

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

    _listener->OnPisteDirty ();
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

    _listener->OnPisteDirty ();
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
