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

#include <string.h>
#include <libxml/encoding.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "util/attribute.hpp"
#include "util/attribute_desc.hpp"
#include "util/filter.hpp"
#include "util/player.hpp"
#include "util/glade.hpp"
#include "network/advertiser.hpp"
#include "actors/team.hpp"
#include "../../tournament.hpp"
#include "../../contest.hpp"

#include "splitting.hpp"

namespace People
{
  const gchar *Splitting::_class_name     = N_ ("Split");
  const gchar *Splitting::_xml_class_name = "PointDeScission";

  Tournament *Splitting::_tournament = NULL;

  // --------------------------------------------------------------------------------
  Splitting::Splitting (StageClass *stage_class)
    : Object ("Splitting"),
    Stage (stage_class),
    PlayersList ("splitting.glade",
                 SORTABLE)
  {
    // Sensitive widgets
    {
      AddSensitiveWidget (_glade->GetWidget ("splitting_move_toolbutton"));
    }

    {
      GSList *attr_list;
      Filter *filter;

      AttributeDesc::CreateExcludingList (&attr_list,
#ifndef DEBUG
                                          "ref",
                                          "plugin_ID",
#endif
                                          "IP",
                                          "password",
                                          "HS",
                                          "attending",
                                          "final_rank",
                                          "global_status",
                                          "indice",
                                          "level",
                                          "workload_rate",
                                          "pool_nr",
                                          "promoted",
                                          "rank",
                                          "status",
                                          "team",
                                          "victories_count",
                                          "bouts_count",
                                          "victories_ratio",
                                          NULL);
      filter = new Filter (GetKlassName (),
                           attr_list);

      filter->ShowAttribute ("exported");
      filter->ShowAttribute ("stage_start_rank");
      filter->ShowAttribute ("name");
      filter->ShowAttribute ("first_name");
      filter->ShowAttribute ("birth_date");
      filter->ShowAttribute ("gender");
      filter->ShowAttribute ("club");
      filter->ShowAttribute ("country");
      filter->ShowAttribute ("licence");

      SetFilter (filter);
      filter->Release ();
    }

    SetAttributeRight ("exported",
                       TRUE);
  }

  // --------------------------------------------------------------------------------
  Splitting::~Splitting ()
  {
  }

  // --------------------------------------------------------------------------------
  void Splitting::Declare ()
  {
    RegisterStageClass (gettext (_class_name),
                        _xml_class_name,
                        CreateInstance,
                        EDITABLE|REMOVABLE);
  }

  // --------------------------------------------------------------------------------
  void Splitting::Garnish ()
  {
  }

  // --------------------------------------------------------------------------------
  void Splitting::Reset ()
  {
    GSList              *current = GetShortList ();
    Player::AttributeId  exported_attr ("exported");

    while (current)
    {
      Player *player = (Player *) current->data;

      player->SetAttributeValue (&exported_attr,
                                 (guint) FALSE);
      Update (player);

      current = g_slist_next (current);
    }
  }

  // --------------------------------------------------------------------------------
  void Splitting::SetHostTournament (Tournament *tournament)
  {
    _tournament = tournament;
  }

  // --------------------------------------------------------------------------------
  Stage *Splitting::CreateInstance (StageClass *stage_class)
  {
    return new Splitting (stage_class);
  }

  // --------------------------------------------------------------------------------
  void Splitting::SaveAttendees (XmlScheme *xml_scheme)
  {
  }

  // --------------------------------------------------------------------------------
  void Splitting::Load (xmlNode *xml_node)
  {
    LoadConfiguration (xml_node);

    RetrieveAttendees ();
  }

  // --------------------------------------------------------------------------------
  void Splitting::OnUnPlugged ()
  {
    PlayersList::Wipe ();
  }

  // --------------------------------------------------------------------------------
  void Splitting::Display ()
  {
    for (GSList *current = GetShortList (); current; current = g_slist_next (current))
    {
      Add ((Player *) current->data);
    }

    OnAttrListUpdated ();
  }

  // --------------------------------------------------------------------------------
  void Splitting::OnLocked ()
  {
    DisableSensitiveWidgets ();
    SetSensitiveState (FALSE);

    if (GetState () == OPERATIONAL)
    {
      Contest             *contest = _contest->Duplicate ();
      GSList              *current = GetShortList ();
      Player::AttributeId  exported_attr ("exported");

      _tournament->Plug (contest,
                         NULL);
      _tournament->Manage (contest);

      for (guint i = 0; current != NULL; i++)
      {
        Player    *player = (Player *) current->data;
        Attribute *attr   = player->GetAttribute (&exported_attr);

        if (attr->GetUIntValue () == TRUE)
        {
          Player *new_player = player->Duplicate ();

          new_player->SetAttributeValue (&exported_attr,
                                         (guint) FALSE);
          contest->AddFencer (new_player,
                              i+1);

          if (player->Is ("Team"))
          {
            Team   *team    = (Team *) player;
            GSList *members = team->GetMemberList ();

            while (members)
            {
              Player *member     = (Player *) members->data;
              Player *new_member = member->Duplicate ();

              contest->AddFencer  (new_member);
              new_member->Release ();

              members = g_slist_next (members);
            }
          }

          new_player->Release ();
        }

        current = g_slist_next (current);
      }
      contest->LatchPlayerList ();
    }
  }

  // --------------------------------------------------------------------------------
  GSList *Splitting::GetCurrentClassification ()
  {
    Player::AttributeId rank_attr_id          ("rank", this);
    Player::AttributeId previous_rank_attr_id ("rank", GetPreviousStage ());
    GList  *current = _player_list;

    while (current)
    {
      Player    *fencer        = (Player *) current->data;
      Attribute *previous_rank = fencer->GetAttribute (&previous_rank_attr_id);

      fencer->SetAttributeValue (&rank_attr_id,
                                 previous_rank->GetUIntValue ());
      current = g_list_next (current);
    }

    return GetRemainingList ();
  }

  // --------------------------------------------------------------------------------
  GSList *Splitting::GetRemainingList ()
  {
    GSList *remaining = CreateCustomList (PresentPlayerFilter, this);

    if (remaining)
    {
      Player::AttributeId attr_id ("stage_start_rank",
                                   this);

      attr_id.MakeRandomReady (_rand_seed);
      remaining = g_slist_sort_with_data (remaining,
                                                   (GCompareDataFunc) Player::Compare,
                                                   &attr_id);
    }

    return remaining;
  }

  // --------------------------------------------------------------------------------
  gboolean Splitting::PresentPlayerFilter (Player      *player,
                                           PlayersList *owner)
  {
    Player::AttributeId  attr_id ("exported");
    Attribute           *attr = player->GetAttribute (&attr_id);

    return (attr->GetUIntValue () == FALSE);
  }

  // --------------------------------------------------------------------------------
  void Splitting::OnUnLocked ()
  {
    EnableSensitiveWidgets ();
    SetSensitiveState (TRUE);

    Reset ();

    OnAttrListUpdated ();
    OnListChanged ();
  }

  // --------------------------------------------------------------------------------
  gchar *Splitting::GetPrintName ()
  {
    return g_strdup_printf ("%s - %s", gettext ("Split"), GetName ());
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_splitting_print_toolbutton_clicked (GtkWidget *widget,
                                                                         Object    *owner)
  {
    Splitting *s = dynamic_cast <Splitting *> (owner);

    s->Print (gettext ("List of fencers to extract"));
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_splitting_filter_toolbutton_clicked (GtkWidget *widget,
                                                                          Object    *owner)
  {
    Splitting *s = dynamic_cast <Splitting *> (owner);

    s->SelectAttributes ();
  }
}
