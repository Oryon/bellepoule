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

#include <stdlib.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <goocanvas.h>

#include "score_collector.hpp"

// --------------------------------------------------------------------------------
ScoreCollector::ScoreCollector (GooCanvas      *canvas,
                                CanvasModule_c *client,
                                OnNewScore_cbk  on_new_score)
{
  _entry_item       = NULL;
  _gtk_entry        = NULL;
  _collecting_point = NULL;
  _canvas           = canvas;
  _client           = client;
  _on_new_score     = on_new_score;
  _is_locked        = FALSE;

  _consistent_focus_color    = g_strdup ("SkyBlue");
  _consistent_normal_color   = g_strdup ("White");
  _unconsistent_focus_color  = g_strdup ("LightCoral");
  _unconsistent_normal_color = g_strdup ("MistyRose");
}

// --------------------------------------------------------------------------------
ScoreCollector::~ScoreCollector ()
{
  Stop ();

  g_free (_consistent_focus_color);
  g_free (_consistent_normal_color);
  g_free (_unconsistent_focus_color);
  g_free (_unconsistent_normal_color);
}

// --------------------------------------------------------------------------------
void ScoreCollector::AddCollectingTrigger (GooCanvasItem *trigger)
{
  if (trigger)
  {
    g_object_set (trigger, "can-focus", TRUE, NULL);

    g_signal_connect (trigger, "button_press_event",
                      G_CALLBACK (on_cell_button_press), _collecting_point);
  }
}

// --------------------------------------------------------------------------------
void ScoreCollector::AddCollectingPoint (GooCanvasItem *point,
                                         GooCanvasItem *score_text,
                                         Match_c       *match,
                                         Player_c      *player,
                                         guint          player_position)
{
  _collecting_point = point;

  AddCollectingTrigger (point);
  AddCollectingTrigger (score_text);

  g_signal_connect (point, "focus_in_event",
                    G_CALLBACK (on_focus_in), this);

  if (player_position == 0)
  {
    match->SetData (this, "goo_rect_A", point);
  }
  else
  {
    match->SetData (this, "goo_rect_B", point);
  }

  g_object_set_data (G_OBJECT (point), "match",  match);
  g_object_set_data (G_OBJECT (point), "player", player);
  g_object_set_data (G_OBJECT (point), "score_text", score_text);
  g_object_set_data (G_OBJECT (point), "next_point",  NULL);

  SetMatchColor (match,
                 _consistent_normal_color,
                 _unconsistent_normal_color);
}

// --------------------------------------------------------------------------------
void ScoreCollector::RemoveCollectingPoints (Match_c *match)
{
  match->SetData (this, "goo_rect_A", NULL);
  match->SetData (this, "goo_rect_B", NULL);
}

// --------------------------------------------------------------------------------
void ScoreCollector::Lock ()
{
  _is_locked = TRUE;
}

// --------------------------------------------------------------------------------
void ScoreCollector::UnLock ()
{
  _is_locked = FALSE;
}

// --------------------------------------------------------------------------------
void ScoreCollector::SetNextCollectingPoint (GooCanvasItem *to,
                                             GooCanvasItem *next)
{
  g_object_set_data (G_OBJECT (to), "next_point",  next);
}

// --------------------------------------------------------------------------------
gboolean ScoreCollector::on_key_press_event (GtkWidget   *widget,
                                             GdkEventKey *event,
                                             gpointer     user_data)
{
  ScoreCollector *collector = (ScoreCollector *) user_data;

  return collector->OnKeyPress (widget,
                                event);
}

// --------------------------------------------------------------------------------
gboolean ScoreCollector::OnKeyPress (GtkWidget   *widget,
                                     GdkEventKey *event)
{
  if ((event->keyval == GDK_Return) || (event->keyval == GDK_Tab))
  {
    GooCanvasItem *next_item = GetNextItem (widget);

    if (next_item)
    {
      goo_canvas_grab_focus (_canvas,
                             next_item);
    }
    else
    {
      goo_canvas_grab_focus (_canvas,
                             goo_canvas_get_root_item (_canvas));
      Stop ();
    }

    return TRUE;
  }
  else if (event->keyval == GDK_Escape)
  {
    goo_canvas_grab_focus (_canvas,
                           goo_canvas_get_root_item (_canvas));
    Stop ();
    return TRUE;
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
GooCanvasItem *ScoreCollector::GetNextItem (GtkWidget *widget)
{
  Match_c       *match     = (Match_c *) g_object_get_data (G_OBJECT (widget), "match");
  GooCanvasItem *next_item = (GooCanvasItem *) g_object_get_data (G_OBJECT (widget), "next_point");

  if (next_item)
  {
    Match_c *next_match = (Match_c *) g_object_get_data (G_OBJECT (next_item), "match");

    if (   (match == next_match)
        && (match->GetWinner () != NULL))
    {
      return NULL;
    }
  }

  return next_item;
}

// --------------------------------------------------------------------------------
void ScoreCollector::OnEntryChanged (GtkWidget *widget)
{
  Match_c  *match  = (Match_c *)  g_object_get_data (G_OBJECT (widget), "match");
  Player_c *player = (Player_c *) g_object_get_data (G_OBJECT (widget), "player");
  gchar    *input  = (gchar *) gtk_entry_get_text (GTK_ENTRY (widget));

  if (match->SetScore (player, input))
  {
    GooCanvasItem *next_item = GetNextItem (widget);

    if (next_item)
    {
      goo_canvas_grab_focus (_canvas,
                             next_item);
    }
    else
    {
      goo_canvas_grab_focus (_canvas,
                             goo_canvas_get_root_item (_canvas));
      Stop ();
    }
  }
}

// --------------------------------------------------------------------------------
void ScoreCollector::SetMatchColor (Match_c *match,
                                    gchar   *consistent_color,
                                    gchar   *unconsitentcolor)
{
  GooCanvasItem *rect;
  gchar         *color_A = consistent_color;
  gchar         *color_B = consistent_color;
  Player_c      *A       = match->GetPlayerA ();
  Player_c      *B       = match->GetPlayerB ();
  Score_c       *score_A = match->GetScore (A);
  Score_c       *score_B = match->GetScore (B);

  if (score_A->IsValid () == false)
  {
    color_A = unconsitentcolor;
  }
  if (score_B->IsValid () == false)
  {
    color_B = unconsitentcolor;
  }

  if (score_A->IsConsistentWith (score_B) == false)
  {
    color_A = unconsitentcolor;
    color_B = unconsitentcolor;
  }

  rect = (GooCanvasItem *) match->GetData (this, "goo_rect_A");
  if (rect)
  {
    g_object_set (rect,
                  "fill-color", color_A,
                  NULL);
  }

  rect = (GooCanvasItem *) match->GetData (this, "goo_rect_B");
  if (rect)
  {
    g_object_set (rect,
                  "fill-color", color_B,
                  NULL);
  }
}

// --------------------------------------------------------------------------------
gboolean ScoreCollector::on_entry_changed (GtkEditable *editable,
                                           gpointer     user_data)
{
  ScoreCollector *collector = (ScoreCollector *) user_data;

  collector->OnEntryChanged (GTK_WIDGET (editable));

  return TRUE;
}

// --------------------------------------------------------------------------------
gboolean ScoreCollector::on_focus_in (GooCanvasItem *goo_rect,
                                      GooCanvasItem *target,
                                      GdkEventFocus *event,
                                      gpointer       data)
{
  ScoreCollector *collector = (ScoreCollector *) data;

  return collector->OnFocusIn (goo_rect);
}

// --------------------------------------------------------------------------------
gboolean ScoreCollector::OnFocusIn (GooCanvasItem *goo_rect)
{
  if (_is_locked == FALSE)
  {
    Match_c  *match  = (Match_c *)  g_object_get_data (G_OBJECT (goo_rect), "match");
    Player_c *player = (Player_c *) g_object_get_data (G_OBJECT (goo_rect), "player");

    Stop ();

    SetMatchColor (match,
                   _consistent_focus_color,
                   _unconsistent_focus_color);

    {
      gpointer next_point   = g_object_get_data (G_OBJECT (goo_rect), "next_point");
      gpointer score_text   = g_object_get_data (G_OBJECT (goo_rect), "score_text");
      Score_c  *score       = match->GetScore (player);
      gchar    *score_image = score->GetImage ();

      _gtk_entry  = gtk_entry_new ();
      gtk_entry_set_text (GTK_ENTRY (_gtk_entry),
                          score_image);
      g_free (score_image);

      g_object_set_data (G_OBJECT (_gtk_entry), "match",  match);
      g_object_set_data (G_OBJECT (_gtk_entry), "player", player);
      g_object_set_data (G_OBJECT (_gtk_entry), "score_text", score_text);
      g_object_set_data (G_OBJECT (_gtk_entry), "next_point", next_point);
    }

    {
      GooCanvasBounds bounds;

      goo_canvas_item_get_bounds ((GooCanvasItem *) goo_rect,
                                  &bounds);

      {
        gdouble x = (bounds.x1 + bounds.x2) / 2;
        gdouble y = (bounds.y1 + bounds.y2) / 2;
        gdouble w = (bounds.x2 - bounds.x1) - 8;
        gdouble h = (bounds.y2 - bounds.y1) - 8;

        _entry_item = goo_canvas_widget_new (goo_canvas_get_root_item (_canvas),
                                             _gtk_entry,
                                             x,
                                             y,
                                             w,
                                             h,
                                             "anchor", GTK_ANCHOR_CENTER,
                                             NULL);
      }

      gtk_widget_grab_focus (_gtk_entry);

      g_signal_connect (GTK_ENTRY (_gtk_entry), "changed",
                        G_CALLBACK (on_entry_changed), this);
      g_signal_connect (_gtk_entry, "key-press-event",
                        G_CALLBACK (on_key_press_event), this);
      _focus_out_handle = g_signal_connect (_gtk_entry, "focus-out-event",
                                            G_CALLBACK (on_focus_out), this);
    }
  }

  return TRUE;
}

// --------------------------------------------------------------------------------
gboolean ScoreCollector::on_focus_out (GtkWidget     *widget,
                                       GdkEventFocus *event,
                                       gpointer       user_data)
{
  ScoreCollector *collector = (ScoreCollector *) user_data;

  return collector->OnFocusOut (widget);
}

// --------------------------------------------------------------------------------
gboolean ScoreCollector::OnFocusOut (GtkWidget *widget)
{
  Match_c     *match  = (Match_c *)  g_object_get_data (G_OBJECT (widget), "match");
  Player_c    *player = (Player_c *) g_object_get_data (G_OBJECT (widget), "player");
  const gchar *input  = gtk_entry_get_text (GTK_ENTRY (_gtk_entry));

  {
    gpointer score_text = g_object_get_data (G_OBJECT (widget), "score_text");

    match->SetScore (player, (gchar *) input);

    if (score_text)
    {
      Score_c *score = match->GetScore (player);

      g_object_set (score_text,
                    "text",
                    score->GetImage (), NULL);
    }

    if (_client && _on_new_score)
    {
      _on_new_score (_client,
                     match,
                     player);
    }
  }

  SetMatchColor (match,
                 _consistent_normal_color,
                 _unconsistent_normal_color);

  return TRUE;
}

// --------------------------------------------------------------------------------
gboolean ScoreCollector::on_cell_button_press (GooCanvasItem  *item,
                                               gboolean       *target,
                                               GdkEventButton *event,
                                               GooCanvasItem  *goo_rect)
{
  GooCanvas *canvas = goo_canvas_item_get_canvas (goo_rect);

  goo_canvas_grab_focus (canvas, goo_rect);

  return FALSE;
}

// --------------------------------------------------------------------------------
void ScoreCollector::Stop ()
{
  if (_entry_item)
  {
    g_signal_handler_disconnect (_gtk_entry,
                                 _focus_out_handle);
    goo_canvas_item_remove (_entry_item);
    _entry_item = NULL;
  }
}

// --------------------------------------------------------------------------------
void ScoreCollector::SetConsistentColors (gchar *normal_color,
                                          gchar *focus_color)
{
  g_free (_consistent_normal_color);
  _consistent_normal_color = g_strdup (normal_color);

  g_free (_consistent_focus_color);
  _consistent_focus_color  = g_strdup (focus_color);
}

// --------------------------------------------------------------------------------
void ScoreCollector::SetUnConsistentColors (gchar *normal_color,
                                            gchar *focus_color)
{
  g_free (_unconsistent_normal_color);
  _unconsistent_normal_color = g_strdup (normal_color);

  g_free (_unconsistent_focus_color);
  _unconsistent_focus_color  = g_strdup (focus_color);
}
