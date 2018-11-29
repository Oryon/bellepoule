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

#include "util/canvas_module.hpp"
#include "network/ring.hpp"
#include "../../stage.hpp"

#include "pillow_dialog.hpp"

class Data;
class Player;

namespace People
{
  class PlayersList;
}

namespace Pool
{
  class Pool;
  class PoolZone;
  class Swapper;

  class Allocator :
    public Stage,
    public CanvasModule,
    public Net::Ring::PartnerListener,
    public PillowDialog::Listener
  {
    public:
      static void Declare ();

      Allocator (StageClass *stage_class);

      void Save (XmlScheme *xml_scheme) override;

      void SaveHeader (XmlScheme *xml_scheme);

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
      void OnLatecomerClicked ();
      void OnNewLatecomerClicked ();
      void OnAbsentClicked ();
      void OnFilterClicked ();
      void OnPrintClicked ();

    private:
      gboolean WarnLocking () override;
      void OnLocked () override;
      void OnUnLocked () override;
      void Reset () override;
      void ClearConfigurations ();
      GSList *GetCurrentClassification () override;
      void LoadConfiguration (xmlNode *xml_node) override;
      void SaveConfiguration (XmlScheme *xml_scheme) override;

    private:
      guint PreparePrint (GtkPrintOperation *operation,
                          GtkPrintContext   *context) override;
      void DrawPage (GtkPrintOperation *operation,
                     GtkPrintContext   *context,
                     gint               page_nr) override;
      gchar *GetPrintName () override;

      void DrawConfig (GtkPrintOperation *operation,
                       GtkPrintContext   *context,
                       gint               page_nr) override;

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
      SensitivityTrigger  *_swapping_sensitivity_trigger;
      People::PlayersList *_fencer_list;
      Swapper             *_swapper;
      gboolean             _has_marshaller;
      PillowDialog        *_latecomer_dialog;
      PillowDialog        *_absent_dialog;

      void Setup ();
      void PopulateFencerList ();
      void CreatePools ();
      void DeletePools ();
      void SetUpCombobox ();
      void Display () override;
      void RefreshDisplay ();
      void Garnish () override;
      void FillPoolTable (PoolZone *zone);
      void DisplayPlayer (Player *player, guint indice, GooCanvasItem *table, PoolZone *zone, GList *layout_list);
      void FixUpTablesBounds ();
      void RegisterConfig (Configuration *config);
      const gchar *GetInputProviderClient () override;

      gboolean OnMessage (Net::Message *message) override;

      void OnPoolRoadmap (Pool         *pool,
                          Net::Message *message);

      void Recall () override;

      void SpreadJobs ();

      void RecallJobs ();

      void FeedParcel (Net::Message *parcel) override;

      gint GetNbMatchs ();

      void OnAttrListUpdated () override;

      gboolean IsOver () override;

      Error *GetError () override;

      void OnPlugged () override;

      void OnUnPlugged () override;

      void OnPartnerJoined (Net::Partner *partner,
                            gboolean      joined) override;

      void DisplaySwapperError ();

      static Stage *CreateInstance (StageClass *stage_class);

      void Load (xmlNode *xml_node) override;

      void SelectConfig (guint nb_pool);

      void ApplyConfig () override;

      void FillInConfig () override;

      Pool *GetPoolOf (GSList *drop_zone);

      PoolZone *GetPoolOf (Player *of);

      void DropObject (Object   *object,
                       DropZone *source_zone,
                       DropZone *target_zone) override;

      Object *GetDropObjectFromRef (guint32 ref,
                                    guint   key) override;

      gboolean DragingIsForbidden (Object *object) override;

      GString *GetFloatingImage (Object *floating_object) override;

      void DumpToHTML (FILE *file) override;

      static void OnSwappingToggled (GtkToggleButton *togglebutton,
                                     Allocator       *allocator);

      gboolean OnAttendeeToggled (PillowDialog *from,
                                  Player       *attendee,
                                  gboolean      attending) override;

      void ShareAttendees (Stage *with) override;

      ~Allocator () override;
  };
}
