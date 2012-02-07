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

#include "data.hpp"
#include "stage.hpp"
#include "canvas_module.hpp"
#include "players_list.hpp"
#include "pool_match_order.hpp"
#include "pool.hpp"

class PoolAllocator : public virtual Stage, public CanvasModule
{
  public:
    static void Init ();

    PoolAllocator (StageClass *stage_class);

    void Save (xmlTextWriter *xml_writer);

    guint GetNbPools ();

    Pool *GetPool (guint index);

    Data *GetMaxScore ();

    gboolean SeedingIsBalanced ();

  public:
    static const gchar *_class_name;
    static const gchar *_xml_class_name;

    void OnComboboxChanged (GtkComboBox *cb);
    void OnSwappingComboboxChanged (GtkComboBox *cb);
    void OnFencerListToggled (gboolean toggled);

  private:
    void OnLocked (Reason reason);
    void OnUnLocked ();
    void OnCanceled ();
    void Wipe ();
    GSList *GetCurrentClassification ();
    void LoadConfiguration (xmlNode *xml_node);
    void SaveConfiguration (xmlTextWriter *xml_writer);
    void OnBeginPrint (GtkPrintOperation *operation,
                       GtkPrintContext   *context);
    void OnDrawPage (GtkPrintOperation *operation,
                     GtkPrintContext   *context,
                     gint               page_nr);

    gboolean OnDragMotion (GtkWidget      *widget,
                           GdkDragContext *drag_context,
                           gint            x,
                           gint            y,
                           guint           time);
    gboolean OnDragDrop (GtkWidget      *widget,
                         GdkDragContext *drag_context,
                         gint            x,
                         gint            y,
                         guint           time);
    void OnDragLeave (GtkWidget      *widget,
                      GdkDragContext *drag_context,
                      guint           time);
    void OnDragDataReceived (GtkWidget        *widget,
                             GdkDragContext   *drag_context,
                             gint              x,
                             gint              y,
                             GtkSelectionData *data,
                             guint             info,
                             guint             time);


    void Focus   (Pool *pool);
    void Unfocus ();

  private:
    typedef struct
    {
      guint _nb_pool;
      guint _size;
      guint _nb_overloaded;
    } Configuration;

    GSList             *_pools_list;
    GSList             *_config_list;
    Configuration      *_best_config;
    Configuration      *_selected_config;
    GooCanvas          *_canvas;
    gboolean            _dragging;
    GooCanvasItem      *_drag_text;
    gdouble             _drag_x;
    gdouble             _drag_y;
    Pool               *_target_pool;
    Pool               *_source_pool;
    Player             *_floating_player;
    GooCanvasItem      *_main_table;
    GtkListStore       *_combobox_store;
    Data               *_swapping;
    Data               *_seeding_balanced;
    AttributeDesc      *_swapping_criteria;
    gdouble             _max_w;
    gdouble             _max_h;
    gdouble             _print_scale;
    gdouble             _page_h;
    guint               _nb_page;
    gboolean            _loaded;
    SensitivityTrigger  _swapping_sensitivity_trigger;
    PlayersList        *_fencer_list;
    GtkTargetList      *_dnd_target_list;
    gint                _nb_matchs;

    void Setup ();
    void PopulateFencerList ();
    void CreatePools ();
    void DeletePools ();
    void SetUpCombobox ();
    void Display ();
    void Garnish ();
    void FillPoolTable (Pool *pool);
    void DisplayPlayer (Player *player, guint indice, GooCanvasItem *table, Pool *pool, GSList *selected_attr);
    void FixUpTablesBounds ();
    void RegisterConfig (Configuration *config);
    const gchar *GetInputProviderClient ();
    gint GetNbMatchs ();

    void OnAttrListUpdated ();

    gboolean IsOver ();

    gboolean OnButtonPress (GooCanvasItem  *item,
                            GooCanvasItem  *target,
                            GdkEventButton *event,
                            Pool           *pool);
    gboolean OnButtonRelease (GooCanvasItem  *item,
                              GooCanvasItem  *target,
                              GdkEventButton *event);
    gboolean OnMotionNotify (GooCanvasItem  *item,
                             GooCanvasItem  *target,
                             GdkEventButton *event);
    gboolean OnEnterNotify (GooCanvasItem  *item,
                            GooCanvasItem  *target,
                            GdkEventButton *event,
                            Pool           *pool);
    gboolean OnLeaveNotify (GooCanvasItem  *item,
                            GooCanvasItem  *target,
                            GdkEventButton *event,
                            Pool           *pool);

    static gboolean on_enter_player (GooCanvasItem  *item,
                                  GooCanvasItem  *target,
                                  GdkEventButton *event,
                                  Pool           *pool);
    static gboolean on_leave_player (GooCanvasItem  *item,
                                  GooCanvasItem  *target,
                                  GdkEventButton *event,
                                  Pool           *pool);
    static gboolean on_button_press (GooCanvasItem  *item,
                                     GooCanvasItem  *target,
                                     GdkEventButton *event,
                                     Pool           *pool);
    static gboolean on_button_release (GooCanvasItem  *item,
                                       GooCanvasItem  *target,
                                       GdkEventButton *event,
                                       PoolAllocator  *pl);
    static gboolean on_motion_notify (GooCanvasItem  *item,
                                      GooCanvasItem  *target,
                                      GdkEventButton *event,
                                      PoolAllocator  *pl);
    static gboolean on_enter_notify (GooCanvasItem  *item,
                                     GooCanvasItem  *target,
                                     GdkEventButton *event,
                                     Pool           *pool);
    static gboolean on_leave_notify (GooCanvasItem  *item,
                                     GooCanvasItem  *target,
                                     GdkEventButton *event,
                                     Pool           *pool);

    void OnPlugged ();

    static Stage *CreateInstance (StageClass *stage_class);

    void Load (xmlNode *xml_node);

    void ApplyConfig ();

    void FillInConfig ();

    ~PoolAllocator ();
};

#endif
