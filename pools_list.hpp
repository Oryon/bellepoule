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

#ifndef pools_list_hpp
#define pools_list_hpp

#include <gtk/gtk.h>

#include "stage.hpp"
#include "canvas_module.hpp"
#include "players_base.hpp"
#include "pool.hpp"

class PoolsList_c : public virtual Stage_c, public CanvasModule_c
{
  public:
     PoolsList_c (gchar *name,
                  PlayersBase_c *players_base);

    void Load (xmlDoc *doc);
    void Save (xmlTextWriter *xml_writer);

    guint   GetNbPools ();
    Pool_c *GetPool    (guint index);

  public:
    static void On_Nb_Pool_Combobox_Changed (GtkWidget *widget,
                                             gpointer  *data);
    static void On_Pool_Size_Combobox_Changed (GtkWidget *widget,
                                               gpointer  *data);

  private:
    void Enter ();
    void OnLocked ();
    void OnUnLocked ();
    void Wipe ();

  private:
    typedef struct
    {
      guint    nb_pool;
      guint    size;
      gboolean has_two_size;
    } Configuration;

    PlayersBase_c *_players_base;
    GSList        *_present_players_list;
    GSList        *_list;
    GSList        *_config_list;
    guint          _pool_size;
    guint          _nb_pools;
    guint          _best_nb_pools;
    GooCanvas     *_canvas;
    gboolean       _dragging;
    GooCanvasItem *_drag_text;
    gdouble        _drag_x;
    gdouble        _drag_y;
    Pool_c        *_target_pool;
    Pool_c        *_source_pool;
    Player_c      *_floating_player;
    GooCanvasItem *_main_table;

    void FillCombobox ();
    void CreatePools ();
    void SetUpCombobox ();
    void Display ();
    void FillPoolTable (Pool_c *pool);
    void FixUpTablesBounds ();

    static gboolean PresentPlayerFilter (Player_c *player);

    void OnPlugged ();
    void OnComboboxChanged (GtkComboBox *cb);

    gboolean OnButtonPress (GooCanvasItem  *item,
                            GooCanvasItem  *target,
                            GdkEventButton *event,
                            Pool_c         *pool);
    gboolean OnButtonRelease (GooCanvasItem  *item,
                              GooCanvasItem  *target,
                              GdkEventButton *event);
    gboolean OnMotionNotify (GooCanvasItem  *item,
                             GooCanvasItem  *target,
                             GdkEventButton *event);
    gboolean OnEnterNotify (GooCanvasItem  *item,
                            GooCanvasItem  *target,
                            GdkEventButton *event,
                            Pool_c         *pool);
    gboolean OnLeaveNotify (GooCanvasItem  *item,
                            GooCanvasItem  *target,
                            GdkEventButton *event,
                            Pool_c         *pool);

    static gboolean on_button_press (GooCanvasItem  *item,
                                     GooCanvasItem  *target,
                                     GdkEventButton *event,
                                     Pool_c         *pool);
    static gboolean on_button_release (GooCanvasItem  *item,
                                       GooCanvasItem  *target,
                                       GdkEventButton *event,
                                       PoolsList_c    *pl);
    static gboolean on_motion_notify (GooCanvasItem  *item,
                                      GooCanvasItem  *target,
                                      GdkEventButton *event,
                                      PoolsList_c    *pl);
    static gboolean on_enter_notify (GooCanvasItem  *item,
                                     GooCanvasItem  *target,
                                     GdkEventButton *event,
                                     Pool_c         *pool);
    static gboolean on_leave_notify (GooCanvasItem  *item,
                                     GooCanvasItem  *target,
                                     GdkEventButton *event,
                                     Pool_c         *pool);

    ~PoolsList_c ();
};

#endif
