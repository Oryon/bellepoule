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

#include "util/attribute.hpp"
#include "util/filter.hpp"
#include "common/schedule.hpp"
#include "common/player.hpp"
#include "common/contest.hpp"

#include "player_factory.hpp"
#include "checkin_supervisor.hpp"

namespace People
{
  const gchar *CheckinSupervisor::_class_name     = N_("Check-in");
  const gchar *CheckinSupervisor::_xml_class_name = "checkin_stage";

  // --------------------------------------------------------------------------------
  CheckinSupervisor::CheckinSupervisor (StageClass *stage_class)
    : Checkin ("checkin.glade",
               "Fencer"),
    Stage (stage_class)
  {
    _use_initial_rank = FALSE;
    _checksum_list    = NULL;

    // Sensitive widgets
    {
      AddSensitiveWidget (_glade->GetWidget ("add_player_button"));
      AddSensitiveWidget (_glade->GetWidget ("remove_player_button"));
      AddSensitiveWidget (_glade->GetWidget ("import_toolbutton"));
      AddSensitiveWidget (_glade->GetWidget ("all_present_button"));
      AddSensitiveWidget (_glade->GetWidget ("all_absent_button"));
    }

    // Fencer
    {
      GSList *attr_list;
      Filter *filter;

      AttributeDesc::CreateExcludingList (&attr_list,
#ifndef DEBUG
                                          "ref",
                                          "start_rank",
#endif
                                          "rank",
                                          "availability",
                                          "participation_rate",
                                          "level",
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
      CreateForm (filter,
                  "Fencer");
      filter->Release ();
    }

    // Team
    {
      GSList *attr_list;
      Filter *filter;

      AttributeDesc::CreateIncludingList (&attr_list,
#ifndef DEBUG
                                          "ref",
#endif
                                          "name",
                                          NULL);
      filter = new Filter (attr_list,
                           this);

      _form->AddPage (filter,
                      "Team");

      filter->Release ();
    }
  }

  // --------------------------------------------------------------------------------
  CheckinSupervisor::~CheckinSupervisor ()
  {
    g_slist_free (_checksum_list);
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::Declare ()
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
  }

  // --------------------------------------------------------------------------------
  guint CheckinSupervisor::PreparePrint (GtkPrintOperation *operation,
                                         GtkPrintContext   *context)
  {
    if (GetStageView (operation) == STAGE_VIEW_CLASSIFICATION)
    {
      return 0;
    }

    return Checkin::PreparePrint (operation,
                                  context);
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::Load (xmlXPathContext *xml_context,
                                const gchar     *from_node)
  {
    LoadList (xml_context,
              from_node,
              "Fencer");
    LoadList (xml_context,
              from_node,
              "Team");
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::OnLoaded ()
  {
    GSList *current = _player_list;

    while (current)
    {
      Player *player = (Player *) current->data;

      if (player->Is ("Team") == FALSE)
      {
        gchar  *team_name = NULL;
        Player *team      = NULL;

        // Team name
        {
          Player::AttributeId  team_attr_id ("team");
          Attribute           *team_attr = player->GetAttribute (&team_attr_id);

          if (team_attr)
          {
            team_name = team_attr->GetStrValue ();
          }
        }

        // Find a team with the given name
        if (team_name && team_name[0])
        {
          Player::AttributeId  name_attr_id ("name");
          Attribute           *name_attr = Attribute::New ("name");

          name_attr->SetValue (team_name);

          team = GetPlayerWithAttribute (&name_attr_id,
                                         name_attr);
          name_attr->Release ();
        }

        player->SetParent (team);
      }
      current = g_slist_next (current);
    }

    _attendees = new Attendees ();
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::OnPlayerLoaded (Player *player)
  {
    Player::AttributeId  start_rank_id ("start_rank");
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
    SaveList (xml_writer,
              "Fencer");
    SaveList (xml_writer,
              "Team");
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
    GSList *current = _player_list;

    while (current)
    {
      Player *p = (Player *) current->data;

      if (p)
      {
        Player::AttributeId  place_attr_id ("final_rank");
        Attribute           *final_rank  = p->GetAttribute (&place_attr_id);

        if (final_rank)
        {
          Player::AttributeId ranking_attr_id ("ranking");

          p->SetAttributeValue (&ranking_attr_id,
                                final_rank->GetUIntValue ());
          p->RemoveAttribute (&place_attr_id);
          Update (p);
        }
      }
      current = g_slist_next (current);
    }
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::UpdateChecksum ()
  {
    // Alphabetic sorting
    {
      Player::AttributeId  name_attr_id       ("name");
      Player::AttributeId  first_name_attr_id ("first_name");
      GSList              *attr_list = NULL;

      attr_list = g_slist_prepend (attr_list, &first_name_attr_id);
      attr_list = g_slist_prepend (attr_list, &name_attr_id);

      _player_list = g_slist_sort_with_data (_player_list,
                                             (GCompareDataFunc) Player::MultiCompare,
                                             attr_list);
      g_slist_free (attr_list);
    }

    g_slist_free (_checksum_list);
    _checksum_list = NULL;

    {
      Player::AttributeId  attr_id ("attending");
      GChecksum           *checksum       = g_checksum_new (G_CHECKSUM_MD5);
      GSList              *current_player = _player_list;

      for (guint ref = 1; current_player; ref++)
      {
        Player *p = (Player *) current_player->data;

        if (p)
        {
          Attribute *attending = p->GetAttribute (&attr_id);

          if (attending && attending->GetUIntValue ())
          {
            _checksum_list = g_slist_prepend (_checksum_list, p);

            // Let the player contribute to the checksum
            {
              gchar *name = p->GetName ();

              name[0] = toupper (name[0]);
              g_checksum_update (checksum,
                                 (guchar *) name,
                                 1);
              g_free (name);
            }

            // Give the player a reference
            if (GetState () == OPERATIONAL)
            {
              p->SetRef (ref);
            }
          }
        }
        current_player = g_slist_next (current_player);
      }

      _checksum_list = g_slist_reverse (_checksum_list);

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

        current_player = g_slist_next (current_player);
      }
    }
    rank_criteria_id->Release ();
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::OnLoadingCompleted ()
  {
    GSList *current = _checksum_list;

    for (guint ref = 1; current; ref++)
    {
      Player *player = (Player *) current->data;

      player->SetRef (ref);
      Update (player);

      current = g_slist_next (current);
    }
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::OnLocked ()
  {
    DisableSensitiveWidgets ();
    SetSensitiveState (FALSE);

    UpdateChecksum ();
    UpdateRanking  ();

    _form->Lock ();
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
    _form->UnLock ();
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::Wipe ()
  {
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::RegisterNewTeam (const gchar *name)
  {
    AttributeDesc *team_desc = AttributeDesc::GetDescFromCodeName ("team");

    team_desc->AddDiscreteValues (name,
                                  name,
                                  NULL,
                                  NULL);
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::OnPlayerEventFromForm (Player            *player,
                                                 Form::PlayerEvent  event)
  {
    if (player->Is ("Team") == FALSE)
    {
      gchar  *team_name;
      Player *team      = NULL;

      // Team name
      {
        Player::AttributeId  team_attr_id ("team");
        Attribute           *team_attr = player->GetAttribute (&team_attr_id);

        team_name = team_attr->GetStrValue ();
      }

      // Find a team with the given name
      if (team_name && team_name[0])
      {
        Player::AttributeId  name_attr_id ("name");
        Attribute           *name_attr = Attribute::New ("name");

        name_attr->SetValue (team_name);

        team = GetPlayerWithAttribute (&name_attr_id,
                                       name_attr);
        name_attr->Release ();

        // Create a team if necessary
        if (team == NULL)
        {
          team = PlayerFactory::CreatePlayer ("Team");

          team->SetName (team_name);
          RegisterNewTeam (team_name);
          Add (team);
        }
      }

      player->SetParent (team);
    }

    if (event == Form::NEW_PLAYER)
    {
      if (player->Is ("Team"))
      {
        Player::AttributeId  attr_id ("name");
        Attribute           *attr      = player->GetAttribute (&attr_id);

        RegisterNewTeam (attr->GetStrValue ());
      }
    }

    Checkin::OnPlayerEventFromForm (player,
                                    event);

  }
}
