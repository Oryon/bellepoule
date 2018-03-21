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
      void Display ();
      void Garnish ();
      void OnLocked ();
      void OnUnLocked ();
      void RetrievePools ();
      void Manage (Pool *pool);

    private:
      void OnPoolSelected (Pool *pool);

      static Stage *CreateInstance (StageClass *stage_class);

      void FillInConfig ();
      void ApplyConfig ();
      Stage *GetInputProvider ();
      gboolean IsOver ();
      gchar *GetError ();

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

      virtual ~Supervisor ();

      void SetPrintPoolDensity ();

      void OnAttrListUpdated ();

      void OnPlugged ();

      void OnUnPlugged ();

      void Load (xmlNode *xml_node);

      void DumpToHTML (FILE *file);

      GSList *GetCurrentClassification ();

      GSList *EvaluateClassification (GSList           *list,
                                      Object           *rank_owner,
                                      GCompareDataFunc  CompareFunction);

      void SetInputProvider (Stage *input_provider);

      gchar *GetPrintName ();

      guint PreparePrint (GtkPrintOperation *operation,
                          GtkPrintContext   *context);

      void DrawPage (GtkPrintOperation *operation,
                     GtkPrintContext   *context,
                     gint               page_nr);

      void OnEndPrint (GtkPrintOperation *operation,
                       GtkPrintContext   *context);

      gboolean OnMessage (Net::Message *message);

      gboolean OnHttpPost (const gchar *command,
                           const gchar **ressource,
                           const gchar *data);

      void OnPoolStatus (Pool *pool);

      static gint CompareCurrentRoundClassification (Player     *A,
                                                     Player     *B,
                                                     Supervisor *pool_supervisor);

      static gint CompareCombinedRoundsClassification (Player     *A,
                                                       Player     *B,
                                                       Supervisor *pool_supervisor);
  };
}
