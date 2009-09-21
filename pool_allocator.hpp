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

#ifndef pools_allocator_hpp
#define pools_allocator_hpp

#include <gtk/gtk.h>

#include "stage.hpp"
#include "canvas_module.hpp"
#include "pool.hpp"

class PoolAllocator_c : public virtual Stage_c, public CanvasModule_c
{
  public:
    static void Init ();

    PoolAllocator_c (gchar *name);

    void Save (xmlTextWriter *xml_writer);

    guint   GetNbPools ();
    Pool_c *GetPool    (guint index);

  public:
    void OnComboboxChanged (GtkComboBox *cb);

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

    static const gchar *_class_name;

    GSList        *_attendees;
    GSList        *_pools_list;
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
    GtkListStore  *_combobox_store;

    void FillCombobox ();
    void CreatePools ();
    void DeletePools ();
    void SetUpCombobox ();
    void Display ();
    void FillPoolTable (Pool_c *pool);
    void FixUpTablesBounds ();

    void OnPlugged ();
    void OnAttrListUpdated ();

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
    static gboolean on_button_release (GooCanvasItem   *item,
                                       GooCanvasItem   *target,
                                       GdkEventButton  *event,
                                       PoolAllocator_c *pl);
    static gboolean on_motion_notify (GooCanvasItem   *item,
                                      GooCanvasItem   *target,
                                      GdkEventButton  *event,
                                      PoolAllocator_c *pl);
    static gboolean on_enter_notify (GooCanvasItem  *item,
                                     GooCanvasItem  *target,
                                     GdkEventButton *event,
                                     Pool_c         *pool);
    static gboolean on_leave_notify (GooCanvasItem  *item,
                                     GooCanvasItem  *target,
                                     GdkEventButton *event,
                                     Pool_c         *pool);

    static Stage_c *CreateInstance (xmlNode *xml_node);

    void Load (xmlNode *xml_node);

    ~PoolAllocator_c ();
};

#endif
