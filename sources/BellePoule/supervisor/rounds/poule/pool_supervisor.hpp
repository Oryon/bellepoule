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

#include "util/module.hpp"
#include "../../stage.hpp"

#include "pool.hpp"

class Data;

namespace Pool
{
  class Allocator;

  class Supervisor :
    public Stage,
    public Module,
    public Pool::StatusListener
  {
    public:
      static void Declare ();

      Supervisor (StageClass *stage_class);

      void OnPrintPoolToolbuttonClicked ();

      void OnPoolSelected (gint index);

      void OnStuffClicked ();

      void OnForRefereesClicked (GtkToggleButton *toggle_button);

    private:
      void Display () override;
      void Garnish () override;
      void OnLocked () override;
      void OnUnLocked () override;
      void RetrievePools ();
      void Manage (Pool *pool);
      void Save (XmlScheme *xml_scheme) override;

    private:
      void OnPoolSelected (Pool *pool);

      static Stage *CreateInstance (StageClass *stage_class);

      void FillInConfig () override;
      void ApplyConfig () override;
      Stage *GetInputProvider () override;
      gboolean IsOver () override;
      Error *GetError () override;

    private:
      static const gchar *_class_name;
      static const gchar *_xml_class_name;

      Object         *_current_round_owner;
      GtkListStore   *_pool_liststore;
      Allocator      *_allocator;
      Pool           *_displayed_pool;
      gboolean        _print_all_pool;
      Classification *_current_round_classification;
      guint           _pool_v_density;
      guint           _pool_h_density;

      ~Supervisor () override;

      void SetPrintPoolDensity ();

      void OnAttrListUpdated () override;

      void OnPlugged () override;

      void OnUnPlugged () override;

      void Load (xmlNode *xml_node) override;

      void DumpToHTML (FILE *file) override;

      GSList *GetCurrentClassification () override;

      GSList *EvaluateClassification (GSList           *list,
                                      Object           *rank_owner,
                                      GCompareDataFunc  CompareFunction);

      void SetInputProvider (Stage *input_provider) override;

      gchar *GetPrintName () override;

      guint PreparePrint (GtkPrintOperation *operation,
                          GtkPrintContext   *context) override;

      void DrawPage (GtkPrintOperation *operation,
                     GtkPrintContext   *context,
                     gint               page_nr) override;

      void OnEndPrint (GtkPrintOperation *operation,
                       GtkPrintContext   *context) override;

      gboolean OnMessage (Net::Message *message) override;

      gboolean OnHttpPost (const gchar *command,
                           const gchar **ressource,
                           const gchar *data);

      void OnPoolStatus (Pool *pool) override;

      static gint CompareCurrentRoundClassification (Player     *A,
                                                     Player     *B,
                                                     Supervisor *pool_supervisor);

      static gint CompareCombinedRoundsClassification (Player     *A,
                                                       Player     *B,
                                                       Supervisor *pool_supervisor);
  };
}
