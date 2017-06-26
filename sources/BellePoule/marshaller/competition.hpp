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

#include "network/message.hpp"
#include "util/module.hpp"
#include "batch.hpp"

class Player;

namespace Marshaller
{
  class Competition : public Module
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

      gboolean BatchIsModifialble (Batch *batch);

      void SetBatchStatus (Batch         *batch,
                           Batch::Status  status);

      void OnSpread ();

    private:
      guint            _id;
      GdkColor        *_gdk_color;
      GList           *_fencer_list;
      gchar           *_weapon;
      GData           *_properties;
      Batch::Listener *_batch_listener;
      GList           *_batches;
      Batch           *_current_batch;
      GtkWidget       *_batch_image;
      GtkWidget       *_spread_button;

      virtual ~Competition ();

      void SetProperty (Net::Message *message,
                        const gchar  *property);

      void SetCurrentBatch (Batch *batch);
  };
}
