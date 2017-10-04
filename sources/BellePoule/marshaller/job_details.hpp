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

#include "actors/players_list.hpp"

namespace Marshaller
{
  class RefereePool;

  class JobDetails : public People::PlayersList
  {
    public:
      struct Listener
      {
        virtual void        OnPlayerAdded   (Player     *player) = 0;
        virtual void        OnPlayerRemoved (Player     *player) = 0;
        virtual JobDetails *GetPairedOf     (JobDetails *of)     = 0;
      };

    public:
      JobDetails (Listener *listener,
                  GList    *player_list,
                  Job      *job,
                  gboolean  dnd_capable);

      void ForceRedraw ();

      static void SetRefereePool (RefereePool *referee_pool);

    private:
      static RefereePool *_referee_pool;
      Listener           *_listener;
      Job                *_job;

      void OnPlugged ();

      virtual ~JobDetails ();

      void OnPlayerRemoved (Player *player);

      void CellDataFunc (GtkTreeViewColumn *tree_column,
                         GtkCellRenderer   *cell,
                         GtkTreeModel      *tree_model,
                         GtkTreeIter       *iter);
    private:
      void OnDragDataGet (GtkWidget        *widget,
                          GdkDragContext   *drag_context,
                          GtkSelectionData *selection_data,
                          guint             key,
                          guint             time);

      Object *GetDropObjectFromRef (guint32 ref,
                                    guint   key);

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
  };
}
