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

#include "util/canvas_module.hpp"
#include "network/message.hpp"
#include "piste.hpp"
#include "batch.hpp"
#include "timeline.hpp"
#include "clock.hpp"

namespace Marshaller
{
  class RefereePool;
  class Lasso;
  class JobBoard;

  class Hall :
    public CanvasModule,
    public Piste::Listener,
    public Batch::Listener,
    public Timeline::Listener,
    public Clock::Listener
  {
    public:
      struct Listener
      {
        virtual void OnExposeWeapon (const gchar *weapon) = 0;
      };

    public:
      Hall (RefereePool *referee_pool,
            Listener    *listener);

      Piste *AddPiste (gdouble tx = 0.0, gdouble ty = 0.0);

      Piste *GetPiste (guint id);

      void RemovePiste (Piste *piste);

      void TranslateSelected (gdouble tx,
                              gdouble ty);

      void RotateSelected ();

      void RemoveSelected ();

      void AlignSelectedOnGrid ();

      void ManageContest (Net::Message *message,
                          GtkNotebook  *notebook);

      void DeleteContest (Net::Message *message);

      void ManageJob (Net::Message *message);

      void ManageFencer (Net::Message *message);

      void DeleteJob (Net::Message *message);

      void DeleteFencer (Net::Message *message);

    private:
      GList       *_piste_list;
      GList       *_selected_list;
      gboolean     _dragging;
      gdouble      _drag_x;
      gdouble      _drag_y;
      GList       *_batch_list;
      RefereePool *_referee_pool;
      Timeline    *_timeline;
      Lasso       *_lasso;
      Clock       *_clock;
      JobBoard    *_job_board;
      Listener    *_listener;

      ~Hall ();

      void OnPlugged ();

      void CancelSelection ();

      void SelectPiste (Piste *piste);

      void UnSelectPiste (Piste *piste);

      Batch *GetBatch (guint id);

      GList *GetFreePisteSlots (GTimeSpan duration);

      EnlistedReferee *GetFreeRefereeFor (GList     *referee_list,
                                          Slot      *slot,
                                          GTimeSpan  duration);

      Object *GetDropObjectFromRef (guint32 ref,
                                    guint   key);

      void DropObject (Object   *object,
                       DropZone *source_zone,
                       DropZone *target_zone);

      void OnNewTime (const gchar *time);

      static gboolean OnButtonPress (GooCanvasItem  *goo_rect,
                                     GooCanvasItem  *target,
                                     GdkEventButton *event,
                                     Hall           *hall);

      static gboolean OnButtonReleased (GooCanvasItem  *goo_rect,
                                        GooCanvasItem  *target,
                                        GdkEventButton *event,
                                        Hall           *hall);

      static gboolean OnMotionNotify (GooCanvasItem  *item,
                                      GooCanvasItem  *target,
                                      GdkEventMotion *event,
                                      Hall           *hall);

      gboolean OnCursorMotion (GdkEventMotion *event);

      void OnPisteButtonEvent (Piste          *piste,
                               GdkEventButton *event);

      void OnPisteDoubleClick (Piste          *piste,
                               GdkEventButton *event);

      void OnPisteMotionEvent (Piste          *piste,
                               GdkEventMotion *event);

      gboolean OnBatchAssignmentRequest (Batch *batch);

      void OnBatchAssignmentCancel (Batch *batch);

      void OnTimelineCursorMoved ();
  };
}
