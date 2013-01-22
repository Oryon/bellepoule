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

#include "common/score_collector.hpp"

// --------------------------------------------------------------------------------
ScoreCollector::ScoreCollector (CanvasModule   *client,
                                OnNewScore_cbk  on_new_score,
                                gboolean        display_match_name)
: Object ("ScoreCollector")
{
  _entry_item         = NULL;
  _gtk_entry          = NULL;
  _collecting_point   = NULL;
  _client             = client;
  _on_new_score       = on_new_score;
  _is_locked          = FALSE;
  _display_match_name = display_match_name;

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
    g_object_set (G_OBJECT (trigger), "can-focus", TRUE, NULL);

    g_signal_connect (G_OBJECT (trigger), "button_press_event",
                      G_CALLBACK (on_cell_button_press), _collecting_point);
  }
}

// --------------------------------------------------------------------------------
void ScoreCollector::AddCollectingPoint (GooCanvasItem *point,
                                         GooCanvasItem *score_text,
                                         Match         *match,
                                         Player        *player)
{
  _collecting_point = point;

  AddCollectingTrigger (point);
  AddCollectingTrigger (score_text);

  g_signal_connect (point, "focus_in_event",
                    G_CALLBACK (on_focus_in), this);
  g_signal_connect (point, "key-press-event",
                    G_CALLBACK (on_goocanvas_key_press_event), this);

  g_object_set_data (G_OBJECT (point), "score_text", score_text);
  g_object_set_data (G_OBJECT (point), "next_point", NULL);

  SetMatch (point,
            match,
            player);
}

// --------------------------------------------------------------------------------
void ScoreCollector::SetMatch (GooCanvasItem *to_point,
                               Match         *match,
                               Player        *player)
{
  if (match)
  {
    if (match->GetPlayerA () == player)
    {
      match->SetData (this, "goo_rect_A", to_point);
    }
    else
    {
      match->SetData (this, "goo_rect_B", to_point);
    }

    g_object_set_data (G_OBJECT (to_point), "match",  match);
    g_object_set_data (G_OBJECT (to_point), "player", player);

    Refresh (match);
  }
}

// --------------------------------------------------------------------------------
void ScoreCollector::Refresh (Match *match)
{
  GooCanvasItem *goo_rect = (GooCanvasItem *) match->GetPtrData (this, "goo_rect_A");

  if (goo_rect)
  {
    if (match == (Match *) g_object_get_data (G_OBJECT (goo_rect), "match"))
    {
      // A
      {
        Score         *score       = match->GetScore (match->GetPlayerA ());
        GooCanvasItem *score_text  = (GooCanvasItem *) g_object_get_data (G_OBJECT (goo_rect), "score_text");
        gchar         *score_image = score->GetImage ();

        g_object_set (score_text,
                      "text", score_image,
                      "fill-color", "black",
                      NULL);
        if (   (score_image[0] == 0)
            && (_display_match_name))
        {
          g_object_set (score_text,
                        "text", match->GetName (),
                        "fill-color", "GainsBoro",
                        NULL);
        }
        g_free (score_image);
      }

      // B
      goo_rect = (GooCanvasItem *) match->GetPtrData (this, "goo_rect_B");
      if (goo_rect)
      {
        Score         *score       = match->GetScore (match->GetPlayerB ());
        GooCanvasItem *score_text  = (GooCanvasItem *) g_object_get_data (G_OBJECT (goo_rect), "score_text");
        gchar         *score_image = score->GetImage ();

        g_object_set (score_text,
                      "text", score_image,
                      "fill-color", "black",
                      NULL);
        if (   (score_image[0] == 0)
            && (_display_match_name))
        {
          g_object_set (score_text,
                        "text", match->GetName (),
                        "fill-color", "GainsBoro",
                        NULL);
        }
        g_free (score_image);
      }

      SetMatchColor (match,
                     _consistent_normal_color,
                     _unconsistent_normal_color);
    }
  }
}

// --------------------------------------------------------------------------------
void ScoreCollector::Wipe (GooCanvasItem *point)
{
  GooCanvasItem *score_text;

  score_text = (GooCanvasItem *) g_object_get_data (G_OBJECT (point), "score_text");
  if (score_text)
  {
    g_object_set (score_text,
                  "text",
                  "", NULL);
  }
}

// --------------------------------------------------------------------------------
void ScoreCollector::RemoveCollectingPoints (Match *match)
{
  if (match)
  {
    match->SetData (this, "goo_rect_A", NULL);
    match->SetData (this, "goo_rect_B", NULL);
  }
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
  if (to)
  {
    g_object_set_data (G_OBJECT (to), "next_point", next);
  }
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
      goo_canvas_grab_focus (goo_canvas_item_get_canvas (next_item),
                             next_item);
    }
    else
    {
      GooCanvasItem *score_text = (GooCanvasItem *) g_object_get_data (G_OBJECT (widget),
                                                                       "score_text");

      goo_canvas_grab_focus (goo_canvas_item_get_canvas (score_text),
                             score_text);
      Stop ();
    }

    return TRUE;
  }
  else if (event->keyval == GDK_Escape)
  {
    GooCanvasItem *score_text = (GooCanvasItem *) g_object_get_data (G_OBJECT (widget),
                                                                     "score_text");

    goo_canvas_grab_focus (goo_canvas_item_get_canvas (score_text),
                           score_text);
    Stop ();
    return TRUE;
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
GooCanvasItem *ScoreCollector::GetNextItem (GtkWidget *widget)
{
  Match         *match     = (Match *) g_object_get_data (G_OBJECT (widget), "match");
  GooCanvasItem *next_item = (GooCanvasItem *) g_object_get_data (G_OBJECT (widget), "next_point");

  while (next_item)
  {
    Match *next_match = (Match *) g_object_get_data (G_OBJECT (next_item), "match");

    if (   (match == next_match)
        && (match->GetWinner () != NULL))
    {
      return NULL;
    }
    else if (next_match->IsDropped () == FALSE)
    {
      return next_item;
    }

    next_item = (GooCanvasItem *) g_object_get_data (G_OBJECT (next_item), "next_point");
  }

  return next_item;
}

// --------------------------------------------------------------------------------
void ScoreCollector::OnEntryChanged (GtkWidget *widget)
{
  Match  *match  = (Match *)  g_object_get_data (G_OBJECT (widget), "match");
  Player *player = (Player *) g_object_get_data (G_OBJECT (widget), "player");
  gchar  *input  = (gchar *) gtk_entry_get_text (GTK_ENTRY (widget));

  if (match->SetScore (player, input))
  {
    GooCanvasItem *next_item = GetNextItem (widget);

    if (next_item)
    {
      goo_canvas_grab_focus (goo_canvas_item_get_canvas (next_item),
                             next_item);
    }
    else
    {
      GooCanvasItem *score_text = (GooCanvasItem *) g_object_get_data (G_OBJECT (widget),
                                                                       "score_text");

      goo_canvas_grab_focus (goo_canvas_item_get_canvas (score_text),
                             score_text);
      Stop ();
    }
  }
}

// --------------------------------------------------------------------------------
void ScoreCollector::SetMatchColor (Match *match,
                                    gchar *consistent_color,
                                    gchar *unconsitentcolor)
{
  if (match)
  {
    GooCanvasItem *rect;
    gchar         *color_A = consistent_color;
    gchar         *color_B = consistent_color;
    Player        *A       = match->GetPlayerA ();
    Player        *B       = match->GetPlayerB ();
    Score         *score_A = match->GetScore (A);
    Score         *score_B = match->GetScore (B);

    if (score_A->IsValid () == FALSE)
    {
      color_A = unconsitentcolor;
    }
    if (score_B->IsValid () == FALSE)
    {
      color_B = unconsitentcolor;
    }

    if (score_A->IsConsistentWith (score_B) == FALSE)
    {
      color_A = unconsitentcolor;
      color_B = unconsitentcolor;
    }

    rect = (GooCanvasItem *) match->GetPtrData (this, "goo_rect_A");
    if (rect)
    {
      g_object_set (rect,
                    "fill-color", color_A,
                    NULL);
    }

    rect = (GooCanvasItem *) match->GetPtrData (this, "goo_rect_B");
    if (rect)
    {
      g_object_set (rect,
                    "fill-color", color_B,
                    NULL);
    }
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
    Match  *match  = (Match *)  g_object_get_data (G_OBJECT (goo_rect), "match");
    Player *player = (Player *) g_object_get_data (G_OBJECT (goo_rect), "player");

    Stop ();

    SetMatchColor (match,
                   _consistent_focus_color,
                   _unconsistent_focus_color);

    if (match && (match->IsDropped () == FALSE))
    {
      {
        gpointer next_point   = g_object_get_data (G_OBJECT (goo_rect), "next_point");
        gpointer score_text   = g_object_get_data (G_OBJECT (goo_rect), "score_text");
        Score    *score       = match->GetScore (player);
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

          _entry_item = goo_canvas_widget_new (goo_canvas_get_root_item (goo_canvas_item_get_canvas (goo_rect)),
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
  Match       *match  = (Match *)  g_object_get_data (G_OBJECT (widget), "match");
  Player      *player = (Player *) g_object_get_data (G_OBJECT (widget), "player");
  const gchar *input  = gtk_entry_get_text (GTK_ENTRY (_gtk_entry));

  {
    gpointer score_text = g_object_get_data (G_OBJECT (widget), "score_text");

    match->SetScore (player, (gchar *) input);

    if (score_text)
    {
      Score *score       = match->GetScore (player);
      gchar *score_image = score->GetImage ();

      if (score_image[0] == 0)
      {
        if (_display_match_name)
        {
          g_object_set (score_text,
                        "text", match->GetName (),
                        "fill-color", "GainsBoro",
                        NULL);
        }
      }
      else
      {
        g_object_set (score_text,
                      "text", score_image,
                      "fill-color", "black",
                      NULL);
      }
      g_free (score_image);
    }

    if (_client && _on_new_score)
    {
      _on_new_score (this,
                     _client,
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
gboolean ScoreCollector::on_goocanvas_key_press_event (GooCanvasItem  *item,
                                                       GooCanvasItem  *target_item,
                                                       GdkEventKey    *event,
                                                       ScoreCollector *score_collector)
{
  if (event->keyval == GDK_Escape)
  {
    goo_canvas_grab_focus (goo_canvas_item_get_canvas (item),
                           goo_canvas_get_root_item (goo_canvas_item_get_canvas (item)));
    score_collector->Stop ();

    {
      Match *match = (Match *) g_object_get_data (G_OBJECT (item), "match");

      if (match)
      {
        score_collector->SetMatchColor (match,
                                        score_collector->_consistent_normal_color,
                                        score_collector->_unconsistent_normal_color);
      }
    }
    return TRUE;
  }
  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean ScoreCollector::on_cell_button_press (GooCanvasItem  *item,
                                               gboolean       *target,
                                               GdkEventButton *event,
                                               GooCanvasItem  *goo_rect)
{
  goo_canvas_grab_focus (goo_canvas_item_get_canvas (goo_rect),
                         goo_rect);

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
void ScoreCollector::SetConsistentColors (const gchar *normal_color,
                                          const gchar *focus_color)
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
