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

#include "player.hpp"
#include "match.hpp"
#include "canvas.hpp"

#include "drop_zone.hpp"

// --------------------------------------------------------------------------------
DropZone::DropZone (Module *container)
  : Object ("DropZone")
{
  _container       = container;
  _referee_list    = NULL;
  _nb_matchs       = 0;
  _nb_matchs_known = FALSE;

  Wipe ();
}

// --------------------------------------------------------------------------------
DropZone::~DropZone ()
{
  while (_referee_list)
  {
    RemoveReferee ((Player *) _referee_list->data);
  }
}

// --------------------------------------------------------------------------------
void DropZone::Wipe ()
{
  _back_rect = NULL;
}

// --------------------------------------------------------------------------------
void DropZone::Draw (GooCanvasItem *root_item)
{
  if (_back_rect)
  {
    goo_canvas_item_lower (_back_rect,
                           NULL);
  }
}

// --------------------------------------------------------------------------------
void DropZone::Redraw (gdouble x,
                       gdouble y,
                       gdouble w,
                       gdouble h)
{
  g_object_set (G_OBJECT (_back_rect),
                "x",      x,
                "y",      y,
                "width",  w,
                "height", h,
                NULL);
}

// --------------------------------------------------------------------------------
void DropZone::GetBounds (GooCanvasBounds *bounds,
                          gdouble          zoom_factor)
{
  if (_back_rect)
  {
    goo_canvas_item_get_bounds (_back_rect,
                                bounds);
    bounds->x1 *= zoom_factor;
    bounds->x2 *= zoom_factor;
    bounds->y1 *= zoom_factor;
    bounds->y2 *= zoom_factor;
  }
  else
  {
    bounds->x1 = 0;
    bounds->x2 = 0;
    bounds->y1 = 0;
    bounds->y2 = 0;
  }
}

// --------------------------------------------------------------------------------
void DropZone::Focus ()
{
  g_object_set (G_OBJECT (_back_rect),
                "fill-color", "Grey",
                NULL);
}

// --------------------------------------------------------------------------------
void DropZone::Unfocus ()
{
  g_object_set (G_OBJECT (_back_rect),
                "fill-color", "White",
                NULL);
}

// --------------------------------------------------------------------------------
void DropZone::AddReferee (Player *referee)
{
  referee->AddMatchs (DropZone::GetNbMatchs ());
  _container->RefreshMatchRate (referee);

  _referee_list = g_slist_prepend (_referee_list,
                                   referee);
}

// --------------------------------------------------------------------------------
void DropZone::RemoveReferee (Player *referee)
{
  referee->RemoveMatchs (DropZone::GetNbMatchs ());
  _container->RefreshMatchRate (referee);

  _referee_list = g_slist_remove (_referee_list,
                                  referee);
}

// --------------------------------------------------------------------------------
guint DropZone::GetNbMatchs ()
{
  if (_nb_matchs_known == FALSE)
  {
    _nb_matchs       = GetNbMatchs ();
    _nb_matchs_known = TRUE;
  }

  return _nb_matchs;
}

// --------------------------------------------------------------------------------
void DropZone::PutInTable (GooCanvasItem *table,
                           guint          row,
                           guint          column)
{
  GSList *current = _referee_list;

  if (current)
  {
    for (guint i = 0; current != NULL; i++)
    {
      Player        *referee = (Player *) current->data;
      GooCanvasItem *item;

      {
        static gchar *referee_icon = NULL;

        if (referee_icon == NULL)
        {
          referee_icon = g_build_filename (_program_path, "resources/glade/referee.png", NULL);
        }

        item = Canvas::PutIconInTable (table,
                                       referee_icon,
                                       row + i,
                                       column);
        g_signal_connect (item, "button_press_event",
                          G_CALLBACK (OnButtonPress), this);
        g_object_set_data (G_OBJECT (item),
                           "DropZone::referee",
                           referee);
      }

      {
        item = Canvas::PutTextInTable (table,
                                       referee->GetName (),
                                       row + i,
                                       column+1);
        Canvas::SetTableItemAttribute (item, "x-align", 1.0);
        Canvas::SetTableItemAttribute (item, "y-align", 0.5);
        g_object_set (item,
                      "font", "Sans Bold Italic 12px",
                      NULL);
        g_signal_connect (item, "button_press_event",
                          G_CALLBACK (OnButtonPress), this);
        g_object_set_data (G_OBJECT (item),
                           "DropZone::referee",
                           referee);
      }

      current = g_slist_next (current);
    }
  }
  else // Goocanvas display issue workaround
  {
    Canvas::PutTextInTable (table,
                            "",
                            row,
                            column+1);
  }
}

// --------------------------------------------------------------------------------
gboolean DropZone::OnItemPress (GooCanvasItem  *item,
                                GdkEventButton *event)
{
  if (   (event->button == 1)
      && (event->type == GDK_BUTTON_PRESS))
  {
    Player *floating_referee = (Player *) g_object_get_data (G_OBJECT (item),
                                                             "DropZone::referee");
    return TRUE;
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean DropZone::OnEnterNotify  (GooCanvasItem    *item,
                                   GooCanvasItem    *target_item,
                                   GdkEventCrossing *event,
                                   DropZone         *drop_zone)
{
  drop_zone->Focus ();
  return TRUE;
}

// --------------------------------------------------------------------------------
gboolean DropZone::OnLeaveNotify  (GooCanvasItem    *item,
                                   GooCanvasItem    *target_item,
                                   GdkEventCrossing *event,
                                   DropZone         *drop_zone)
{
  drop_zone->Unfocus ();
  return TRUE;
}

// --------------------------------------------------------------------------------
gboolean DropZone::OnButtonPress (GooCanvasItem  *item,
                                  GooCanvasItem  *target,
                                  GdkEventButton *event,
                                  DropZone       *drop_zone)
{
  return drop_zone->OnItemPress (item,
                                 event);
}
