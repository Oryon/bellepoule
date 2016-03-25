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

#include "util/canvas_module.hpp"
#include "util/data.hpp"
#include "util/player.hpp"
#include "actors/players_list.hpp"
#include "../../stage.hpp"

#include "pool.hpp"
#include "swapper.hpp"
#include "pool_zone.hpp"

namespace Pool
{
  class Allocator :
    public virtual Stage,
    public CanvasModule,
    public Pool::RoadmapListener
  {
    public:
      static void Declare ();

      Allocator (StageClass *stage_class);

      void Save (xmlTextWriter *xml_writer);

      void SaveHeader (xmlTextWriter *xml_writer);

      guint GetNbPools ();

      Pool *GetPool (guint index);

      PoolZone *GetZone (guint index);

      Data *GetMaxScore ();

      gboolean SeedingIsBalanced ();

      guint GetBiggestPoolSize ();

    public:
      static const gchar *_class_name;
      static const gchar *_xml_class_name;

      void OnComboboxChanged (GtkComboBox *cb);
      void OnFencerListToggled (gboolean toggled);
      void OnFilterClicked ();
      void OnPrintClicked ();
      void OnMarshallerClicked ();

    private:
      void OnLocked ();
      void OnUnLocked ();
      void OnCanceled ();
      void Reset ();
      GSList *GetCurrentClassification ();
      void LoadConfiguration (xmlNode *xml_node);
      void SaveConfiguration (xmlTextWriter *xml_writer);

    private:
      guint PreparePrint (GtkPrintOperation *operation,
                          GtkPrintContext   *context);
      void DrawPage (GtkPrintOperation *operation,
                     GtkPrintContext   *context,
                     gint               page_nr);
      gchar *GetPrintName ();

      void DrawConfig (GtkPrintOperation *operation,
                       GtkPrintContext   *context,
                       gint               page_nr);

    private:
      typedef struct
      {
        guint _nb_pool;
        guint _size;
        guint _nb_overloaded;
      } Configuration;

      static GdkPixbuf    *_warning_pixbuf;
      static GdkPixbuf    *_moved_pixbuf;
      GSList              *_config_list;
      Configuration       *_best_config;
      Configuration       *_selected_config;
      GooCanvas           *_canvas;
      GooCanvasItem       *_main_table;
      GtkListStore        *_combobox_store;
      Data                *_swapping;
      Data                *_seeding_balanced;
      GSList              *_swapping_criteria_list;
      gdouble              _max_w;
      gdouble              _max_h;
      gdouble              _print_scale;
      gdouble              _page_h;
      guint                _nb_page;
      gboolean             _loaded;
      SensitivityTrigger   _swapping_sensitivity_trigger;
      People::PlayersList *_fencer_list;
      Swapper             *_swapper;

      void Setup ();
      void PopulateFencerList ();
      void CreatePools ();
      void DeletePools ();
      void SetUpCombobox ();
      void Display ();
      void RefreshDisplay ();
      void Garnish ();
      void FillPoolTable (PoolZone *zone);
      void DisplayPlayer (Player *player, guint indice, GooCanvasItem *table, PoolZone *zone, GSList *layout_list);
      void FixUpTablesBounds ();
      void RegisterConfig (Configuration *config);
      const gchar *GetInputProviderClient ();

      void OnMessage (Net::Message *message);

      void OnPoolRoadmap (Pool         *pool,
                          Net::Message *message);

      void Spread ();

      gint GetNbMatchs ();

      void OnAttrListUpdated ();

      gboolean IsOver ();

      gchar *GetError ();

      void OnPlugged ();

      void OnUnPlugged ();

      void DisplaySwapperError ();
      static Stage *CreateInstance (StageClass *stage_class);

      void Load (xmlNode *xml_node);

      void ApplyConfig ();

      void FillInConfig ();

      Pool *GetPoolOf (GSList *drop_zone);

      void DropObject (Object   *object,
                       DropZone *source_zone,
                       DropZone *target_zone);

      Object *GetDropObjectFromRef (guint32 ref,
                                    guint   key);

      gboolean DragingIsForbidden (Object *object);

      GString *GetFloatingImage (Object *floating_object);

      void DumpToHTML (FILE *file);

      static void OnSwappingToggled (GtkToggleButton *togglebutton,
                                     Allocator       *allocator);
      virtual ~Allocator ();
  };
}

#endif
