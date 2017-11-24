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

#pragma once

#include <gtk/gtk.h>
#include <goocanvas.h>

#include "util/object.hpp"

class Match;
class Player;
class CanvasModule;

class ScoreCollector : public Object
{
  public:
    typedef void (*OnNewScore_cbk) (ScoreCollector *score_collector,
                                    CanvasModule   *client,
                                    Match          *match,
                                    Player         *player);

  public:
    ScoreCollector (CanvasModule   *client,
                    OnNewScore_cbk  on_new_score,
                    gboolean        display_match_name = TRUE);

    void AddCollectingPoint (GooCanvasItem *point,
                             GooCanvasItem *score_text,
                             Match         *match,
                             Player        *player);

    void SetMatch (GooCanvasItem *to_point,
                   Match         *match,
                   Player        *player);

    void Refresh (Match *match);

    void Wipe (GooCanvasItem *point);

    void RemoveCollectingPoints (Match *match);

    void Lock ();

    void UnLock ();

    void AddCollectingTrigger (GooCanvasItem *trigger);

    void SetNextCollectingPoint (GooCanvasItem *to,
                                 GooCanvasItem *next);

    void SetConsistentColors (const gchar *normal_color,
                              const gchar *focus_color);

    void SetUnConsistentColors (gchar *normal_color,
                                gchar *focus_colorg);

  private:
    GooCanvasItem  *_entry_item;
    GooCanvasItem  *_collecting_point;
    GtkWidget      *_gtk_entry;
    CanvasModule   *_client;
    OnNewScore_cbk  _on_new_score;
    gboolean        _is_locked;
    gulong          _focus_out_handle;
    gchar          *_consistent_focus_color;
    gchar          *_consistent_normal_color;
    gchar          *_unconsistent_focus_color;
    gchar          *_unconsistent_normal_color;
    gboolean        _display_match_name;

    static gboolean on_cell_button_press (GooCanvasItem  *item,
                                          gboolean       *target,
                                          GdkEventButton *event,
                                          GooCanvasItem  *goo_rect);
    static gboolean on_focus_in (GooCanvasItem *goo_rect,
                                 GooCanvasItem *target,
                                 GdkEventFocus *event,
                                 gpointer       data);
    static gboolean on_focus_out (GtkWidget     *widget,
                                  GdkEventFocus *event,
                                  gpointer       user_data);
    static gboolean on_goocanvas_key_press_event (GooCanvasItem *item,
                                                  GooCanvasItem *target_item,
                                                  GdkEventKey   *event,
                                                  ScoreCollector *score_collector);
    static gboolean  on_entry_changed (GtkEditable *editable,
                                       gpointer     user_data);
    static gboolean on_key_press_event (GtkWidget   *widget,
                                        GdkEventKey *event,
                                        gpointer     user_data);

    void SetMatchColor (Match *match,
                        gchar *consistent_color,
                        gchar *unconsitentcolor);

    void Stop ();

    GooCanvasItem *GetNextItem (GtkWidget *widget);

    gboolean OnFocusIn  (GooCanvasItem *goo_rect);
    gboolean OnFocusOut (GtkWidget *widget);
    void     OnEntryChanged (GtkWidget *widget);
    gboolean OnKeyPress (GtkWidget   *widget,
                         GdkEventKey *event);

    virtual ~ScoreCollector ();
};
