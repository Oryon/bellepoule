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
#include "timeline.hpp"

// --------------------------------------------------------------------------------
Timeline::Timeline ()
  : CanvasModule ("timeline.glade",
                  "canvas_scrolled_window")
{
  _batch_list = NULL;
}

// --------------------------------------------------------------------------------
Timeline::~Timeline ()
{
  g_list_free (_batch_list);
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
void Timeline::Redraw ()
{
  Wipe ();

  {
    GtkAllocation  allocation;
    GList         *current_batch = _batch_list;

    gtk_widget_get_allocation (GetRootWidget (),
                               &allocation);

    goo_canvas_set_bounds (GetCanvas (),
                           0.0,
                           0.0,
                           allocation.width,
                           allocation.height);

    goo_canvas_rect_new (GetRootItem (),
                         allocation.width/3,
                         0.0,
                         1.0,
                         allocation.height,
                         "fill-color", "black",
                         NULL);

    for (guint i = 0; current_batch != NULL; i++)
    {
      Batch *batch       = (Batch *) current_batch->data;
      gchar *color       = gdk_color_to_string (batch->GetColor ());
      GList *current_job = batch->GetScheduledJobs ();

      for (guint j = 0; current_job != NULL; j++)
      {
        goo_canvas_rect_new (GetRootItem (),
                             j*20.0,
                             i*5.0,
                             allocation.width/5,
                             5.0,
                             "fill-color",   color,
                             "stroke-color", "white",
                             NULL);

        current_job = g_list_next (current_job);
      }

      g_free (color);

      current_batch = g_list_next (current_batch);
    }
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_canvas_scrolled_window_size_allocate (GtkWidget    *widget,
                                                                         GdkRectangle *allocation,
                                                                         Object       *object)
{
  Timeline *timeline = dynamic_cast <Timeline *> (object);

  timeline->Redraw ();
}
