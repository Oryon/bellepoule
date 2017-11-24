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

#include "piste.hpp"

namespace Marshaller
{
  const gdouble Piste::_W          = 160.0;
  const gdouble Piste::_H          = 20.0;
  const gdouble Piste::_BORDER_W   = 0.5;
  const gdouble Piste::_RESOLUTION = 10.0;

  // --------------------------------------------------------------------------------
  Piste::Piste (GooCanvasItem *parent,
                Module        *container,
                Listener      *listener)
    : DropZone (container)
  {
    _button_press_time = 0;

    _blocked = FALSE;

    _horizontal = TRUE;
    _listener   = NULL;
    _slots      = NULL;

    _listener = listener;

    _job_board = new JobBoard ();

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
                                            _BORDER_W,
                                            0.0,
                                            _H-(2.0*_BORDER_W),
                                            "fill-color-rgba", 0xd3d3d360,
                                            "line-width",      0.0,
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

    // Cone
    {
      gchar     *icon_file = g_build_filename (Global::_share_dir, "resources", "glade", "images", "VLC.png", NULL);
      GdkPixbuf *pixbuf    = gdk_pixbuf_new_from_file_at_size (icon_file, 15, 17, NULL);

      _cone = goo_canvas_image_new (_root_item,
                                    pixbuf,
                                    0.0,
                                    0.0,
                                    NULL);
      g_object_unref (pixbuf);
      g_free (icon_file);

      Canvas::VAlign (_cone,
                      Canvas::MIDDLE,
                      _root_item,
                      Canvas::MIDDLE);
      Canvas::HAlign (_cone,
                      Canvas::MIDDLE,
                      _root_item,
                      Canvas::MIDDLE);

      MonitorEvent (_cone);

      g_object_set (G_OBJECT (_cone),
                    "visibility", GOO_CANVAS_ITEM_INVISIBLE,
                    NULL);
    }

    // Shutter
    {
      _shutter = goo_canvas_rect_new (_root_item,
                                      0.0, 0.0,
                                      _W, _H,
                                      "fill-color-rgba", 0xf3f3f3c0,
                                      "line-width",      0.0,
                                      NULL);
      MonitorEvent (_shutter);

      g_object_set (G_OBJECT (_shutter),
                    "visibility", GOO_CANVAS_ITEM_INVISIBLE,
                    NULL);
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
    FreeFullGList (Slot, _slots);

    goo_canvas_item_remove (_root_item);
    g_free (_color);
    g_free (_focus_color);

    if (_display_time)
    {
      g_date_time_unref (_display_time);
    }

    _job_board->Release ();

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
  void Piste::DisplayAtTime (GDateTime *time)
  {
    if (_display_time)
    {
      g_date_time_unref (_display_time);
    }
    _display_time = g_date_time_add (time, 0);

    CleanDisplay  ();
    OnSlotUpdated (GetSlotAt (time));
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

    return NULL;
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

    return NULL;
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

    g_object_set (G_OBJECT (_shutter),
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
    g_object_set (G_OBJECT (_progress_item),
                  "width", 0.0,
                  NULL);

    if (slot && slot->TimeIsInside (_display_time))
    {
      GString     *match_name   = g_string_new ("");
      const gchar *last_name    = NULL;
      GList       *referee_list = slot->GetRefereeList ();

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
        GList *current = slot->GetJobList ();

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

      {
        GDateTime *start_time = slot->GetStartTime ();
        GTimeSpan  duration   = slot->GetDuration ();
        GDateTime *now        = g_date_time_new_now_local ();
        GTimeSpan  elapsed    = g_date_time_difference (now, start_time);
        gdouble    progress   = (elapsed * _W) / duration;

        if (elapsed > 0)
        {
          g_object_set (G_OBJECT (_progress_item),
                        "width", MIN (progress, _W),
                        NULL);
        }
        else
        {
          g_object_set (G_OBJECT (_progress_item),
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

    if (slot && (slot->GetJobList () == NULL))
    {
      GList *node = g_list_find (_slots,
                                 slot);

      if (node)
      {
        _slots = g_list_delete_link (_slots,
                                     node);
      }
      slot->Release ();
    }
  }

  // --------------------------------------------------------------------------------
  void Piste::OnSlotLocked (Slot *slot)
  {
    _slots = g_list_insert_sorted (_slots,
                                   slot,
                                   (GCompareFunc) Slot::CompareAvailbility);
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
            slot->RemoveJob (job);
            job->ResetRoadMap ();
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
      while (slots)
      {
        Slot *slot = (Slot *) slots->data;

        _job_board->AddJobs (slot->GetJobList ());

        slots = g_list_next (slots);
      }

      _job_board->Display ();
      _job_board->Clean ();
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
