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
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "common/contest.hpp"
#include "common/player.hpp"
#include "common/tournament.hpp"

#include "splitting.hpp"

namespace People
{
  const gchar *Splitting::_class_name     = N_ ("Split");
  const gchar *Splitting::_xml_class_name = "PointDeScission";

  Tournament *Splitting::_tournament = NULL;

  // --------------------------------------------------------------------------------
  Splitting::Splitting (StageClass *stage_class)
    : Stage (stage_class),
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
#endif
                                          "IP",
                                          "HS",
                                          "attending",
                                          "availability",
                                          "final_rank",
                                          "global_status",
                                          "indice",
                                          "level",
                                          "participation_rate",
                                          "pool_nr",
                                          "promoted",
                                          "rank",
                                          "splitting_start_rank",
                                          "status",
                                          "team",
                                          "victories_ratio",
                                          NULL);
      filter = new Filter (attr_list,
                           this);

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
  void Splitting::Save (xmlTextWriter *xml_writer)
  {
    xmlTextWriterStartElement (xml_writer,
                               BAD_CAST _xml_class_name);

    Stage::SaveConfiguration (xml_writer);

    xmlTextWriterEndElement (xml_writer);
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
    GSList *shortlist = _attendees->GetShortList ();

    while (shortlist)
    {
      Player *player = (Player *) shortlist->data;

      Add (player);
      shortlist = g_slist_next (shortlist);
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
      GSList              *current = _attendees->GetShortList ();
      Player::AttributeId  exported_attr ("exported");

      _tournament->Manage (contest);

      for (guint i = 0; current != NULL; i++)
      {
        Player    *player;
        Attribute *attr;

        player = (Player *) current->data;
        attr   = player->GetAttribute (&exported_attr);
        if (attr->GetUIntValue () == TRUE)
        {
          Player *new_player = player->Duplicate ();

          new_player->SetAttributeValue (&exported_attr,
                                         (guint) FALSE);
          contest->AddFencer (new_player,
                              i+1);
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
    GSList *current = _player_list;

    while (current)
    {
      Player    *fencer        = (Player *) current->data;
      Attribute *previous_rank = fencer->GetAttribute (&previous_rank_attr_id);

      fencer->SetAttributeValue (&rank_attr_id,
                                 previous_rank->GetIntValue ());
      current = g_slist_next (current);
    }

    return g_slist_copy (_player_list);
  }

  // --------------------------------------------------------------------------------
  GSList *Splitting::GetOutputShortlist ()
  {
    GSList *result = CreateCustomList (PresentPlayerFilter, this);

    if (result)
    {
      Player::AttributeId attr_id ("stage_start_rank",
                                   this);

      attr_id.MakeRandomReady (_rand_seed);
      result = g_slist_sort_with_data (result,
                                       (GCompareDataFunc) Player::Compare,
                                       &attr_id);
    }
    return result;
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

    {
      GSList              *current = _attendees->GetShortList ();
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

    OnAttrListUpdated ();
    OnListChanged ();
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
