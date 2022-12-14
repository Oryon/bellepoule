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

#include <libxml/xpath.h>

#include "util/module.hpp"
#include "batch_panel.hpp"

class Player;
class Batch;

namespace Marshaller
{
  class Competition : public Module,
                      public BatchPanel::Listener
  {
    public:
      Competition (guint            id,
                   Batch::Listener *listener);

      Batch *ManageBatch (Net::Message *message);

      void DeleteBatch (Net::Message *message);

      Batch *GetBatch (guint id);

      GList *GetBatches ();

      void AttachTo (GtkNotebook *to);

      guint GetId ();

      void SetProperties (Net::Message *message);

      GdkColor *GetColor ();

      GData *GetProperties ();

      const gchar *GetWeaponCode ();

      void ManageFencer (Net::Message *message);

      Player *GetFencer (guint ref);

      void DeleteFencer (Net::Message *message);

      gboolean BatchIsModifiable (Batch *batch);

      void OnNewBatchStatus (Batch *batch);

      void Freeze ();

      void UnFreeze ();

    private:
      guint            _id;
      GdkColor        *_gdk_color;
      GList           *_fencer_list;
      gchar           *_weapon;
      GData           *_properties;
      GList           *_batches;
      Batch::Listener *_batch_listener;
      GtkWidget       *_batch_image;
      GtkTable        *_batch_table;
      Batch           *_current_batch;
      GtkRadioButton  *_radio_group;

      virtual ~Competition ();

      void SetProperty (Net::Message *message,
                        const gchar  *property);

      void ExposeBatch (Batch *batch,
                        guint  expected_jobs,
                        guint  ready_jobs);

      void MaskBatch (Batch *batch);

      BatchPanel *GetBatchPanel (Batch *batch);

      void OnBatchSelected (Batch *batch) override;

      gboolean LoadFencer (xmlXPathContext *xml_context,
                           Net::Message    *message,
                           const gchar     *path,
                           const gchar     *code_name);

      Player *LoadNode (xmlNode      *xml_node,
                        Net::Message *message,
                        const gchar  *code_name);

      void DeleteBatch (Batch *batch);
  };
}
