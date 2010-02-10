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

#ifndef score_collector_hpp
#define score_collector_hpp

#include <gtk/gtk.h>
#include <goocanvas.h>

#include "object.hpp"
#include "canvas_module.hpp"
#include "match.hpp"
#include "player.hpp"

class ScoreCollector : public Object_c
{
  public:
    typedef void (*OnNewScore_cbk) (ScoreCollector *score_collector,
                                    CanvasModule_c *client,
                                    Match_c        *match,
                                    Player_c       *player);

  public:
    ScoreCollector (CanvasModule_c *client,
                    OnNewScore_cbk  on_new_score);

    void AddCollectingPoint (GooCanvasItem *point,
                             GooCanvasItem *score_text,
                             Match_c       *match,
                             Player_c      *player,
                             guint          player_position);

    void SetMatch (GooCanvasItem *to_point,
                   Match_c       *match,
                   Player_c      *player,
                   guint          player_position);

    void Refresh (Match_c *match);

    void Wipe (GooCanvasItem *point);

    void RemoveCollectingPoints (Match_c *match);

    void Lock ();

    void UnLock ();

    void AddCollectingTrigger (GooCanvasItem *trigger);

    void SetNextCollectingPoint (GooCanvasItem *to,
                                 GooCanvasItem *next);

    void SetConsistentColors (gchar *normal_color,
                              gchar *focus_color);

    void SetUnConsistentColors (gchar *normal_color,
                                gchar *focus_colorg);

  private:
    GooCanvasItem  *_entry_item;
    GooCanvasItem  *_collecting_point;
    GtkWidget      *_gtk_entry;
    CanvasModule_c *_client;
    OnNewScore_cbk  _on_new_score;
    gboolean        _is_locked;
    gulong          _focus_out_handle;
    gchar          *_consistent_focus_color;
    gchar          *_consistent_normal_color;
    gchar          *_unconsistent_focus_color;
    gchar          *_unconsistent_normal_color;

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
    static gboolean  on_entry_changed (GtkEditable *editable,
                                       gpointer     user_data);
    static gboolean on_key_press_event (GtkWidget   *widget,
                                        GdkEventKey *event,
                                        gpointer     user_data);

    void SetMatchColor (Match_c *match,
                        gchar   *consistent_color,
                        gchar   *unconsitentcolor);

    void Stop ();

    GooCanvasItem *GetNextItem (GtkWidget *widget);

    gboolean OnFocusIn  (GooCanvasItem *goo_rect);
    gboolean OnFocusOut (GtkWidget *widget);
    void     OnEntryChanged (GtkWidget *widget);
    gboolean OnKeyPress (GtkWidget   *widget,
                         GdkEventKey *event);

    ~ScoreCollector ();
};

#endif

