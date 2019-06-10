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

#include "util/object.hpp"

namespace Marshaller
{
  class Batch;

  class BatchPanel : public Object
  {
    public:
      struct Listener
      {
        virtual void OnBatchSelected (Batch *batch) = 0;
      };

    public:
      BatchPanel (GtkTable       *table,
                  GtkRadioButton *radio_group,
                  Batch          *batch,
                  Listener       *listener);

      void Expose (guint expected_jobs,
                   guint ready_jobs);

      void Mask ();

      gboolean IsVisible ();

      void SetActive ();

      void RefreshStatus ();

    private:
      Listener  *_listener;
      Batch     *_batch;
      GtkWidget *_separator;
      GtkWidget *_label;
      GtkWidget *_radio;
      GtkWidget *_progress;
      GtkWidget *_status;

      ~BatchPanel () override;

      static void OnToggled (GtkToggleButton *button,
                             BatchPanel      *panel);
  };
}
