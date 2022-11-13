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

#include "util/dnd_config.hpp"
#include "util/attribute_desc.hpp"
#include "util/filter.hpp"
#include "util/player.hpp"
#include "actors/team.hpp"

#include "job.hpp"
#include "referee_pool.hpp"
#include "enlisted_referee.hpp"
#include "slot.hpp"
#include "affinities.hpp"

#include "job_details.hpp"

namespace Marshaller
{
  RefereePool *JobDetails::_referee_pool = nullptr;

  // --------------------------------------------------------------------------------
  JobDetails::JobDetails (Listener *listener,
                          GList    *player_list,
                          Job      *job,
                          gboolean  dnd_capable)
    : Object ("JobDetails"),
    PlayersList ("details.glade", nullptr, SORTABLE)
  {
    _listener = listener;
    _job      = job;

    {
      GSList *attr_list;
      Filter *filter;

      AttributeDesc::CreateExcludingList (&attr_list,
#ifndef DEBUG
                                          "ref",
                                          "plugin_ID",
#endif
                                          "incident",
                                          "IP",
                                          "password",
                                          "cyphered_password",
                                          "HS",
                                          "attending",
                                          "exported",
                                          "global_status",
                                          "indice",
                                          "level",
                                          "workload_rate",
                                          "pool_nr",
                                          "stage_start_rank",
                                          "promoted",
                                          "rank",
                                          "status",
                                          "team",
                                          "victories_count",
                                          "bouts_count",
                                          "victories_ratio",
                                          "score_quest",
                                          "tiebreaker_quest",
                                          "nb_matchs",
                                          "strip",
                                          "time",
                                          NULL);
      filter = new Filter (GetKlassName (),
                           attr_list);

      filter->ShowAttribute ("name");
      filter->ShowAttribute ("club");
      filter->ShowAttribute ("league");
      filter->ShowAttribute ("country");

      SetFilter (filter);
      filter->Release ();
    }

    SelectTreeMode ();

    {
      GList *current = player_list;

      while (current)
      {
        Player *player= (Player *) current->data;

        Add (player);

        if (player->Is ("Team"))
        {
          Team   *team    = (Team *) player;
          GSList *members = team->GetMemberList ();

          while (members)
          {
            Add ((Player *) members->data);

            members = g_slist_next (members);
          }
        }

        current = g_list_next (current);
      }
    }

    SetPopupVisibility ("PlayersList::CopyAction",
                        FALSE);

    if (dnd_capable)
    {
      _dnd_config->AddTarget ("bellepoule/referee", GTK_TARGET_SAME_APP|GTK_TARGET_OTHER_WIDGET);

      _dnd_config->SetOnAWidgetDest (_tree_view,
                                     GDK_ACTION_COPY);

      ConnectDndDest (GTK_WIDGET (_tree_view));
    }
  }

  // --------------------------------------------------------------------------------
  JobDetails::~JobDetails ()
  {
  }

  // --------------------------------------------------------------------------------
  void JobDetails::ForceRedraw ()
  {
    gtk_widget_queue_draw (GetRootWidget ());
  }

  // --------------------------------------------------------------------------------
  void JobDetails::OnPlugged ()
  {
    OnAttrListUpdated ();
    gtk_widget_show_all (GetRootWidget ());
  }

  // --------------------------------------------------------------------------------
  void JobDetails::OnPlayerRemoved (Player *player)
  {
    _listener->OnPlayerRemoved (player);
  }

  // --------------------------------------------------------------------------------
  void JobDetails::OnDragDataGet (GtkWidget        *widget,
                                  GdkDragContext   *drag_context,
                                  GtkSelectionData *selection_data,
                                  guint             key,
                                  guint             time)
  {
    GList   *selected   = RetreiveSelectedPlayers ();
    Player  *fencer     = (Player *) selected->data;
    guint32  fencer_ref = fencer->GetRef ();

    gtk_selection_data_set (selection_data,
                            gtk_selection_data_get_target (selection_data),
                            32,
                            (guchar *) &fencer_ref,
                            sizeof (fencer_ref));

    g_list_free (selected);
  }

  // --------------------------------------------------------------------------------
  Object *JobDetails::GetDropObjectFromRef (guint32 ref,
                                            guint   key)
  {
    return _referee_pool->GetReferee (ref);
  }

  // --------------------------------------------------------------------------------
  gboolean JobDetails::OnDragMotion (GtkWidget      *widget,
                                     GdkDragContext *drag_context,
                                     gint            x,
                                     gint            y,
                                     guint           time)
  {
    if (_job->GetSlot ())
    {
      if (Module::OnDragMotion (widget,
                                drag_context,
                                x,
                                y,
                                time))
      {
        EnlistedReferee *referee      = (EnlistedReferee *) _dnd_config->GetFloatingObject ();
        Slot            *referee_slot = referee->GetAvailableSlotFor (_job->GetSlot (),
                                                                      _job->GetRegularDuration ());
        if (referee_slot)
        {
          referee_slot->Release ();
          gdk_drag_status  (drag_context,
                            GDK_ACTION_DEFAULT,
                            time);
          return FALSE;
        }
      }
    }

    gdk_drag_status  (drag_context,
                      GDK_ACTION_PRIVATE,
                      time);

    return TRUE;
  }

  // --------------------------------------------------------------------------------
  gboolean JobDetails::OnDragDrop (GtkWidget      *widget,
                                   GdkDragContext *drag_context,
                                   gint            x,
                                   gint            y,
                                   guint           time)
  {
    if (Module::OnDragDrop (widget,
                            drag_context,
                            x,
                            y,
                            time))
    {
      EnlistedReferee *referee      = (EnlistedReferee *) _dnd_config->GetFloatingObject ();
      Slot            *job_slot     = _job->GetSlot ();
      Slot            *referee_slot = referee->GetAvailableSlotFor (job_slot,
                                                                    _job->GetRegularDuration ());
      if (referee_slot)
      {
        Listener *listener = _listener;

        job_slot->TailWith (referee_slot,
                            _job->GetRegularDuration ());
        job_slot->AddReferee (referee);
        listener->OnPlayerAdded (referee);

        referee_slot->Release ();
      }
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void JobDetails::SetRefereePool (RefereePool *referee_pool)
  {
    _referee_pool = referee_pool;
  }

  // --------------------------------------------------------------------------------
  void JobDetails::CellDataFunc (GtkTreeViewColumn *tree_column,
                                 GtkCellRenderer   *cell,
                                 GtkTreeModel      *tree_model,
                                 GtkTreeIter       *iter)
  {
    JobDetails *paired_details = _listener->GetPairedOf (this);
    GList      *paired_list    = paired_details->GetList ();
    Player     *player         = GetPlayer (tree_model, iter);
    Affinities *affinities     = (Affinities *) player->GetPtrData (nullptr,
                                                                    "affinities");
    g_object_set (cell,
                  "cell-background-set", FALSE,
                  NULL);

    while (paired_list)
    {
      Player     *paired_player     = (Player *) paired_list->data;
      Affinities *paired_affinities = (Affinities *) paired_player->GetPtrData (nullptr, "affinities");
      guint       kinship           = affinities->KinshipWith (paired_affinities);

      if (kinship > 0)
      {
        GList         *affinity_names = Affinities::GetTitles ();
        AttributeDesc *cell_attribute = (AttributeDesc *) g_object_get_data (G_OBJECT (tree_column),
                                                                             "PlayersList::AttributeDesc");

        for (guint i = 0; affinity_names != nullptr; i++)
        {
          if (g_strcmp0 (cell_attribute->_code_name, (gchar *) affinity_names->data) == 0)
          {
            if (kinship & 1<<i)
            {
              g_object_set (cell,
                            "cell-background-gdk", Affinities::GetColor (cell_attribute->_code_name),
                            "cell-background-set", TRUE,
                            NULL);
            }
          }
          affinity_names = g_list_next (affinity_names);
        }
      }
      paired_list = g_list_next (paired_list);
    }
  }
}
