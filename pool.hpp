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

#ifndef pool_hpp
#define pool_hpp

#include <gtk/gtk.h>
#include <goocanvas.h>

#include "stage.hpp"
#include "canvas_module.hpp"
#include "players_base.hpp"

class Pool_c : public CanvasModule_c
{
  public:
    Pool_c (guint number);

    void  AddPlayer    (Player_c *player);
    void  RemovePlayer (Player_c *player);
    guint GetNbPlayers ();
    guint GetNumber    ();
    void  ResetMatches ();
    void  Lock         ();
    void  SetMaxScore  (guint max_score);

    Player_c *GetPlayer (guint i);

    gchar *GetName ();

    void Save  (xmlTextWriter *xml_writer);
    void Load  (xmlNode       *xml_node,
                GSList        *player_list);

    void SetRandSeed (guint32 seed);

    static gint ComparePlayer (Player_c *A,
                               Player_c *B,
                               guint     client_seed);

  private:
    guint          _max_score;
    guint          _rand_seed;
    guint          _number;
    GSList        *_player_list;
    GooCanvasItem *_entry_item;
    GtkWidget     *_gtk_entry;
    GSList        *_match_list;
    gchar         *_name;
    gboolean       _is_over;

  private:
    static gboolean on_cell_button_press (GooCanvasItem  *item,
                                          gboolean       *target,
                                          GdkEventButton *event,
                                          gpointer        data);
    static gboolean on_focus_in (GooCanvasItem *item,
                                 GooCanvasItem *target,
                                 GdkEventFocus *event,
                                 gpointer       data);
    static gboolean on_focus_out (GtkWidget     *widget,
                                  GdkEventFocus *event,
                                  gpointer       user_data);
    static void on_entry_changed (GtkEditable *editable,
                                  gpointer     user_data);
    static gboolean on_key_press_event (GtkWidget   *widget,
                                        GdkEventKey *event,
                                        gpointer     user_data);

  private:
    void SetMatchColor (Match_c *match,
                        gchar   *consistent_color,
                        gchar   *unconsitentcolor);

    gboolean OnFocusIn  (GooCanvasItem *item);
    gboolean OnFocusOut (GtkWidget *widget);
    void     OnEntryChanged (GtkWidget *widget);
    gboolean OnKeyPress (GtkWidget   *widget,
                         GdkEventKey *event);

    void OnBeginPrint (GtkPrintOperation *operation,
                       GtkPrintContext   *context);
    void OnDrawPage (GtkPrintOperation *operation,
                     GtkPrintContext   *context,
                     gint               page_nr);

    void OnPlugged ();

    Match_c *GetMatch (Player_c *A,
                       Player_c *B);

    void RefreshScoreData ();

    void RefreshDashBoard ();

    ~Pool_c ();
};

#endif

