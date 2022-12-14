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
#include <util/object.hpp>
#include <util/module.hpp>

class Match;

namespace Quest
{
  class Hall : public Module
  {
    public:
      struct Listener
      {
        virtual void     OnMatchSelected (Match *match) = 0;
        virtual gboolean MatchIsFinished (Match *match) = 0;
      };

    public:
      Hall (Listener *listener);

      void Clear ();

      void SetPisteCount (guint count);

      guint BookPiste (Match *owner);

      void FreePiste (Match *owner);

      void SetStatusIcon (Match       *match,
                          const gchar *stock_icon);

      void Dump () override;

      void OnPisteSelected ();

    private:
      Listener     *_listener;
      GtkListStore *_store;

      ~Hall ();

      gboolean GetIter (Match       *match,
                        GtkTreeIter *iter);
  };
}
