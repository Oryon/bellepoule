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
#include "util/json_file.hpp"
#include "network/message.hpp"
#include "piste.hpp"
#include "batch.hpp"
#include "timeline.hpp"
#include "clock.hpp"
#include "job_board.hpp"

namespace Marshaller
{
  class Competition;
  class RefereePool;
  class Lasso;

  class Hall :
    public CanvasModule,
    public Piste::Listener,
    public Batch::Listener,
    public Timeline::Listener,
    public JobBoard::Listener,
    public Clock::Listener,
    public JsonFile::Listener,
    public DndConfig::Listener
  {
    public:
      struct Listener
      {
        virtual void OnExposeWeapon (const gchar *weapon) = 0;
        virtual void OnBlur         () = 0;
        virtual void OnUnBlur       () = 0;
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

      void ManageCompetition (Net::Message *message,
                              GtkNotebook  *notebook);
      void DeleteCompetition (Net::Message *message);

      void ManageBatch (Net::Message *message);
      void DeleteBatch (Net::Message *message);

      void ManageJob (Net::Message *message);
      void DeleteJob (Net::Message *message);

      void ManageFencer (Net::Message *message);
      void DeleteFencer (Net::Message *message);

      void OnNewWarningPolicy ();

    private:
      GList       *_piste_list;
      GList       *_selected_list;
      gboolean     _dragging;
      gdouble      _drag_x;
      gdouble      _drag_y;
      GList       *_competition_list;
      RefereePool *_referee_pool;
      Timeline    *_timeline;
      Lasso       *_lasso;
      Clock       *_clock;
      Listener    *_listener;
      guint        _redraw_timeout;
      Job         *_floating_job;
      guint        _referee_key;
      guint        _job_key;
      JsonFile    *_json_file;

      ~Hall ();

      void OnPlugged ();

      void CancelSelection ();

      void SetToolBarSensitivity ();

      void SelectPiste (Piste *piste);

      void UnSelectPiste (Piste *piste);

      Competition *GetCompetition (guint id);

      GList *GetFreePisteSlots (GTimeSpan duration);

      static gint CompareReferee (EnlistedReferee *a,
                                  EnlistedReferee *b,
                                  GList           *fencer_list);

      gboolean SetFreeRefereeFor (GList *referee_list,
                                  Slot  *slot,
                                  Job   *job);

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

      static void OnConeToggled (GtkToggleToolButton *widget,
                                 Hall                *hall);

      gboolean OnCursorMotion (GdkEventMotion *event);

      void OnPisteButtonEvent (Piste          *piste,
                               GdkEventButton *event);

      void OnPisteMotionEvent (Piste          *piste,
                               GdkEventMotion *event);

      void OnPisteDirty ();

      gboolean OnBatchAssignmentRequest (Batch *batch);

      void OnBatchAssignmentCancel (Batch *batch);

      void OnTimelineCursorMoved ();

      void OnJobBoardUpdated (Competition *competition);

      void OnJobBoardFocus (guint focus);

      void OnJobMove (Job *job);

      gboolean DroppingIsAllowed (Object   *floating_object,
                                  DropZone *in_zone);

      gboolean DroppingIsAllowed (Object *floating_object,
                                  Piste  *piste);

      static gboolean RedrawCbk (Hall *hall);

      void ScheduleRedraw ();

      void StopRedraw ();

      gboolean Redraw ();

      void OnDragAndDropEnd ();

      void StopMovingJob ();

      void OnDragDataReceived (GtkWidget        *widget,
                               GdkDragContext   *drag_context,
                               gint              x,
                               gint              y,
                               GtkSelectionData *data,
                               guint             key,
                               guint             time);

      void FeedJsonBuilder (JsonBuilder *builder);

      gboolean ReadJson (JsonReader *reader);
  };
}
