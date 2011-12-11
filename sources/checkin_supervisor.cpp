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

#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <ctype.h>

#include "attribute.hpp"
#include "schedule.hpp"
#include "player.hpp"
#include "filter.hpp"

#include "checkin_supervisor.hpp"

const gchar *CheckinSupervisor::_class_name     = N_("Check-in");
const gchar *CheckinSupervisor::_xml_class_name = "checkin_stage";

// --------------------------------------------------------------------------------
CheckinSupervisor::CheckinSupervisor (StageClass  *stage_class)
: Checkin ("checkin.glade",
           "Tireur"),
  Stage (stage_class)
{
  _use_initial_rank = FALSE;

  // Sensitive widgets
  {
    AddSensitiveWidget (_glade->GetWidget ("add_player_button"));
    AddSensitiveWidget (_glade->GetWidget ("remove_player_button"));
    AddSensitiveWidget (_glade->GetWidget ("import_toolbutton"));
    AddSensitiveWidget (_glade->GetWidget ("all_present_button"));
    AddSensitiveWidget (_glade->GetWidget ("all_absent_button"));
  }

  {
    GSList *attr_list;
    Filter *filter;

    AttributeDesc::CreateList (&attr_list,
#ifndef DEBUG
                               "ref",
                               "start_rank",
#endif
                               "participation_rate",
                               "status",
                               "global_status",
                               "previous_stage_rank",
                               "exported",
                               "final_rank",
                               "victories_ratio",
                               "indice",
                               "pool_nr",
                               "HS",
                               NULL);
    filter = new Filter (attr_list,
                         this);

    filter->ShowAttribute ("attending");
    filter->ShowAttribute ("rank");
    filter->ShowAttribute ("name");
    filter->ShowAttribute ("first_name");
    filter->ShowAttribute ("birth_date");
    filter->ShowAttribute ("gender");
    filter->ShowAttribute ("club");
    filter->ShowAttribute ("league");
    filter->ShowAttribute ("country");
    filter->ShowAttribute ("ranking");
    filter->ShowAttribute ("licence");

    SetFilter  (filter);
    CreateForm (filter);
    filter->Release ();
  }
}

// --------------------------------------------------------------------------------
CheckinSupervisor::~CheckinSupervisor ()
{
}

// --------------------------------------------------------------------------------
void CheckinSupervisor::Init ()
{
  RegisterStageClass (gettext (_class_name),
                      _xml_class_name,
                      CreateInstance,
                      0);
}

// --------------------------------------------------------------------------------
Stage *CheckinSupervisor::CreateInstance (StageClass *stage_class)
{
  return new CheckinSupervisor (stage_class);
}

// --------------------------------------------------------------------------------
void CheckinSupervisor::Load (xmlNode *xml_node)
{
  LoadList (xml_node);
}

// --------------------------------------------------------------------------------
void CheckinSupervisor::Load (xmlXPathContext *xml_context,
                              const gchar     *from_node)
{
  LoadList (xml_context,
            from_node);
}

// --------------------------------------------------------------------------------
void CheckinSupervisor::OnLoaded ()
{
  _attendees = new Attendees ();
}

// --------------------------------------------------------------------------------
void CheckinSupervisor::OnPlayerLoaded (Player *player)
{
  Player::AttributeId  start_rank_id    ("start_rank");
  Attribute           *start_rank = player->GetAttribute (&start_rank_id);

  if (start_rank)
  {
    Player::AttributeId previous_rank_id ("previous_stage_rank", this);

    UseInitialRank ();
    player->SetAttributeValue (&previous_rank_id,
                               start_rank->GetUIntValue ());
  }
}

// --------------------------------------------------------------------------------
void CheckinSupervisor::Save (xmlTextWriter *xml_writer)
{
  SaveList (xml_writer);
}

// --------------------------------------------------------------------------------
void CheckinSupervisor::OnListChanged ()
{
  Checkin::OnListChanged ();
  UpdateRanking ();
}

// --------------------------------------------------------------------------------
void CheckinSupervisor::UseInitialRank ()
{
  _use_initial_rank = TRUE;
}

// --------------------------------------------------------------------------------
void CheckinSupervisor::ConvertFromBaseToResult ()
{
  // This method aims to deal with the strange FIE xml specification.
  // Two different file formats are used. One for preparation one for result.
  // Both are almost similar except that they use a same keyword ("classement")
  // for a different meanning.

  for (guint i = 0; i < g_slist_length (_player_list); i++)
  {
    Player *p = (Player *) g_slist_nth_data (_player_list, i);

    if (p)
    {
      Player::AttributeId  place_attr_id ("final_rank");
      Attribute           *final_rank  = p->GetAttribute (&place_attr_id);

      if (final_rank)
      {
        Player::AttributeId ranking_attr_id ("ranking");

        p->SetAttributeValue (&ranking_attr_id,
                              final_rank->GetUIntValue ());
        p->SetAttributeValue (&place_attr_id,
                              1);
      }
    }
  }
  UpdateRanking ();
}

// --------------------------------------------------------------------------------
void CheckinSupervisor::UpdateChecksum ()
{
  {
    Player::AttributeId attr_id ("ref");

    attr_id.MakeRandomReady (1);
    _player_list = g_slist_sort_with_data (_player_list,
                                           (GCompareDataFunc) Player::Compare,
                                           &attr_id);
  }

  {
    Player::AttributeId  attr_id ("attending");
    GChecksum           *checksum       = g_checksum_new (G_CHECKSUM_MD5);
    GSList              *current_player = _player_list;

    while (current_player)
    {
      Player *p;

      p = (Player *) current_player->data;
      if (p)
      {
        Attribute *attending;

        attending = p->GetAttribute (&attr_id);
        if (attending && attending->GetUIntValue ())
        {
          gchar *name = p->GetName ();

          name[0] = toupper (name[0]);
          g_checksum_update (checksum,
                             (guchar *) name,
                             1);
          g_free (name);
        }
      }
      current_player = g_slist_next (current_player);
    }

    {
      gchar short_checksum[9];

      strncpy (short_checksum,
               g_checksum_get_string (checksum),
               sizeof (short_checksum));
      short_checksum[8] = 0;

      _rand_seed = strtoul (short_checksum, NULL, 16);
    }

    g_checksum_free (checksum);
  }
}

// --------------------------------------------------------------------------------
void CheckinSupervisor::UpdateRanking ()
{
  guint                nb_player = g_slist_length (_player_list);
  Player::AttributeId *rank_criteria_id;

  UpdateChecksum ();

  {
    if (_use_initial_rank)
    {
      rank_criteria_id = new Player::AttributeId ("previous_stage_rank",
                                                  this);
    }
    else
    {
      rank_criteria_id = new Player::AttributeId ("ranking");
    }

    rank_criteria_id->MakeRandomReady (_rand_seed);
    _player_list = g_slist_sort_with_data (_player_list,
                                           (GCompareDataFunc) Player::Compare,
                                           rank_criteria_id);
  }

  {
    Player *previous_player = NULL;
    GSList *current_player  = _player_list;
    gint    previous_rank   = 0;
    guint   nb_present      = 1;

    while (current_player)
    {
      Player    *p;
      Attribute *attending;

      p = (Player *) current_player->data;

      {
        Player::AttributeId attr_id ("attending");

        attending = p->GetAttribute (&attr_id);
      }

      {
        Player::AttributeId previous_rank_id ("previous_stage_rank", this);
        Player::AttributeId rank_id          ("rank", this);

        if (attending && attending->GetUIntValue ())
        {
          if (   previous_player
              && (Player::Compare (previous_player, p, rank_criteria_id) == 0))
          {
            p->SetAttributeValue (&previous_rank_id,
                                  previous_rank);
            p->SetAttributeValue (&rank_id,
                                  previous_rank);
          }
          else
          {
            p->SetAttributeValue (&previous_rank_id,
                                  nb_present);
            p->SetAttributeValue (&rank_id,
                                  nb_present);
            previous_rank = nb_present;
          }
          previous_player = p;
          nb_present++;
        }
        else
        {
          p->SetAttributeValue (&previous_rank_id,
                                nb_player);
          p->SetAttributeValue (&rank_id,
                                nb_player);
        }
      }
      Update (p);

      current_player  = g_slist_next (current_player);
    }
  }
  rank_criteria_id->Release ();
}

// --------------------------------------------------------------------------------
void CheckinSupervisor::OnLocked (Reason reason)
{
  DisableSensitiveWidgets ();
  SetSensitiveState (FALSE);
}

// --------------------------------------------------------------------------------
GSList *CheckinSupervisor::GetCurrentClassification ()
{
  GSList *result = CreateCustomList (PresentPlayerFilter);

  if (result)
  {
    Player::AttributeId attr_id ("rank");

    attr_id.MakeRandomReady (_rand_seed);
    result = g_slist_sort_with_data (result,
                                     (GCompareDataFunc) Player::Compare,
                                     &attr_id);

    result = g_slist_reverse (result);

    _attendees->SetGlobalList (result);
  }
  return result;
}

// --------------------------------------------------------------------------------
gboolean CheckinSupervisor::IsOver ()
{
  return TRUE;
}

// --------------------------------------------------------------------------------
void CheckinSupervisor::OnUnLocked ()
{
  EnableSensitiveWidgets ();
  SetSensitiveState (TRUE);
}

// --------------------------------------------------------------------------------
void CheckinSupervisor::Wipe ()
{
}
