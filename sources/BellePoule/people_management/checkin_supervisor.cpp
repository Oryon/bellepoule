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
#include "application/schedule.hpp"
#include "util/canvas.hpp"
#include "util/player.hpp"
#include "application/contest.hpp"

#include "fencer.hpp"
#include "player_factory.hpp"
#include "checkin_supervisor.hpp"
#include "rank_importer.hpp"

namespace People
{
  const gchar *CheckinSupervisor::_class_name     = N_("Check-in");
  const gchar *CheckinSupervisor::_xml_class_name = "checkin_stage";

  // --------------------------------------------------------------------------------
  CheckinSupervisor::CheckinSupervisor (StageClass *stage_class)
    : Checkin ("checkin.glade",
               "Fencer",
               "Team"),
    Stage (stage_class)
  {
    _checksum_list = NULL;

    _manual_classification  = NULL;
    _default_classification = NULL;
    _minimum_team_size      = NULL;

    // Sensitive widgets
    {
      AddSensitiveWidget (_glade->GetWidget ("add_player_button"));
      AddSensitiveWidget (_glade->GetWidget ("remove_player_button"));
      AddSensitiveWidget (_glade->GetWidget ("import_toolbutton"));
      AddSensitiveWidget (_glade->GetWidget ("ranking_toolbutton"));
      AddSensitiveWidget (_glade->GetWidget ("all_present_button"));
      AddSensitiveWidget (_glade->GetWidget ("all_absent_button"));
      AddSensitiveWidget (_glade->GetWidget ("team_table"));
      AddSensitiveWidget (_glade->GetWidget ("teamsize_entry"));
    }

    // Fencer
    {
      GSList *attr_list;
      Filter *filter;

      AttributeDesc::CreateExcludingList (&attr_list,
#ifndef DEBUG
                                          "ref",
#endif
                                          "IP",
                                          "HS",
                                          "availability",
                                          "exported",
                                          "final_rank",
                                          "global_status",
                                          "indice",
                                          "level",
                                          "participation_rate",
                                          "pool_nr",
                                          "stage_start_rank",
                                          "promoted",
                                          "rank",
                                          "status",
                                          "victories_ratio",
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
                                          "ranking",
                                          NULL);
      filter = new Filter (attr_list,
                           this);

      _form->AddPage (filter,
                      "Team");

      filter->Release ();
    }

    {
      _null_team = new NullTeam ();
      _null_team->SetName ("** Sans équipe **");
      RegisterNewTeam (_null_team);
      Add (_null_team);
    }

    {
      _dnd_target = _dnd_config->CreateTarget ("bellepoule/fencer", GTK_TARGET_SAME_WIDGET);
      _dnd_config->CreateTargetTable ();

      ConnectDndSource (GTK_WIDGET (_tree_view));
      ConnectDndDest   (GTK_WIDGET (_tree_view));
    }
    SetPasteVisibility (TRUE);
  }

  // --------------------------------------------------------------------------------
  CheckinSupervisor::~CheckinSupervisor ()
  {
    g_slist_free (_checksum_list);
    _null_team->Release ();
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::Declare ()
  {
    RegisterStageClass (gettext (_class_name),
                        _xml_class_name,
                        CreateInstance,
                        EDITABLE);
  }

  // --------------------------------------------------------------------------------
  Stage *CheckinSupervisor::CreateInstance (StageClass *stage_class)
  {
    return new CheckinSupervisor (stage_class);
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::SetTeamData (Data *minimum_team_size,
                                       Data *manual_classification,
                                       Data *default_classification)
  {
    _minimum_team_size      = minimum_team_size;
    _default_classification = default_classification;
    _manual_classification  = manual_classification;
    FillInConfig ();
    ApplyConfig ();
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::Load (xmlNode *xml_node)
  {
  }

  // --------------------------------------------------------------------------------
  gboolean CheckinSupervisor::PlayerIsPrintable (Player *player)
  {
    if (player->Is ("Team"))
    {
      Team   *team    = (Team *) player;
      GSList *current = team->GetMemberList ();

      while (current)
      {
        Player *member = (Player *) current->data;

        if (Checkin::PlayerIsPrintable (member))
        {
          return TRUE;
        }

        current = g_slist_next (current);
      }

      return FALSE;
    }
    else
    {
      return Checkin::PlayerIsPrintable (player);
    }
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
  void CheckinSupervisor::DrawConfig (GtkPrintOperation *operation,
                                      GtkPrintContext   *context,
                                      gint               page_nr)
  {
    {
      gchar *text = g_strdup_printf ("%s : %d",
                                     gettext ("Expected fencers"),
                                     g_slist_length (_player_list));

      DrawConfigLine (operation,
                      context,
                      text);

      g_free (text);
    }

    if (_contest->IsTeamEvent ())
    {
      gchar *text = g_strdup_printf ("%s : %s",
                                     gettext ("Team size"),
                                     _minimum_team_size->GetString ());

      DrawConfigLine (operation,
                      context,
                      text);

      g_free (text);
    }
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::LoadConfiguration (xmlNode *xml_node)
  {
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::Load (xmlXPathContext *xml_context,
                                const gchar     *from_node)
  {
    LoadConfiguration (NULL);

    LoadList (xml_context,
              from_node,
              "Team");

    LoadList (xml_context,
              from_node,
              "Fencer");

    _attendees = new Attendees ();
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::OnPlayerLoaded (Player *player,
                                          Player *owner)
  {
    if (player->Is ("Team"))
    {
      RegisterNewTeam ((Team *) player);
    }
    else
    {
      Fencer *fencer = (Fencer *) player;

      if (owner)
      {
        fencer->SetTeam ((Team *) owner);
      }
      else
      {
        fencer->SetTeam (_null_team);
      }
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
  void CheckinSupervisor::SavePlayer (xmlTextWriter *xml_writer,
                                      const gchar   *player_class,
                                      Player        *player)
  {
    if (player != _null_team)
    {
      if (player->Is ("Team"))
      {
        Team *team = (Team *) player;

        team->EnableMemberSaving (_contest->IsTeamEvent ());
      }

      if (_contest->IsTeamEvent ())
      {
        if ((strcmp (player_class, "Fencer") == 0) && player->Is ("Fencer"))
        {
          Fencer *fencer = (Fencer *) player;
          Team   *team   = fencer->GetTeam ();

          if (team != _null_team)
          {
            return;
          }
        }
      }

      Checkin::SavePlayer (xml_writer,
                           player_class,
                           player);
    }
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

    // FFE xml files may have teams with attending attribute set!
    // By default everybody is absent.
    {
      GSList *current = _player_list;

      while (current)
      {
        Player *player = (Player *) current->data;

        if (player->Is ("Team"))
        {
          Team *team = (Team *) player;

          team->SetAttendingFromMembers  ();
          team->SetAttributesFromMembers ();
          Update (team);
        }
        current = g_slist_next (current);
      }
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
      guint                ref            = 1;
      guint                absent_ref     = 10000;

      while (current_player)
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
              ref++;
            }
          }
          else
          {
            p->SetRef (absent_ref);
            absent_ref++;
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
      rank_criteria_id = new Player::AttributeId ("ranking");

      rank_criteria_id->MakeRandomReady (_rand_seed);
    }

    _player_list = g_slist_sort_with_data (_player_list,
                                           (GCompareDataFunc) Player::Compare,
                                           rank_criteria_id);

    {
      Player *previous_player  = NULL;
      GSList *current_player   = _player_list;
      gint    stage_start_rank = 0;
      guint   nb_present       = 1;

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
          Player::AttributeId stage_start_rank_id ("stage_start_rank", this);
          Player::AttributeId rank_id             ("rank", this);

          if (attending && attending->GetUIntValue ())
          {
            if (   previous_player
                && (Player::Compare (previous_player, p, rank_criteria_id) == 0))
            {
              p->SetAttributeValue (&stage_start_rank_id,
                                    stage_start_rank);
              p->SetAttributeValue (&rank_id,
                                    stage_start_rank);
            }
            else
            {
              p->SetAttributeValue (&stage_start_rank_id,
                                    nb_present);
              p->SetAttributeValue (&rank_id,
                                    nb_present);
              stage_start_rank = nb_present;
            }
            previous_player = p;
            nb_present++;
          }
          else
          {
            p->SetAttributeValue (&stage_start_rank_id,
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
  void CheckinSupervisor::ApplyConfig ()
  {
    Stage::ApplyConfig ();

    ApplyConfig (NULL);
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::FillInConfig ()
  {
    Stage::FillInConfig ();

    if (_manual_classification)
    {
      if (_manual_classification->_value)
      {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("manual_radiobutton")),
                                      TRUE);
      }
      else
      {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("derived_radiobutton")),
                                      TRUE);
      }
    }

    if (_default_classification)
    {
      gchar *text = g_strdup_printf ("%d", _default_classification->_value);

      gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("default_classification_entry")),
                          text);
      g_free (text);
    }

    if (_minimum_team_size)
    {
      GtkAdjustment *adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (_glade->GetWidget ("teamsize_entry")));

      if (adj)
      {
        gtk_adjustment_set_value (adj,
                                  _minimum_team_size->_value);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::ApplyConfig (Team *team)
  {
    if (_manual_classification && _minimum_team_size && _default_classification)
    {
      _manual_classification->_value = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("manual_radiobutton")));

      {
        GtkEntry *w = GTK_ENTRY (_glade->GetWidget ("default_classification_entry"));

        if (w)
        {
          gchar *value = (gchar *) gtk_entry_get_text (w);

          if (value)
          {
            _default_classification->_value = atoi (value);
          }
        }
      }

      {
        GtkEntry *w = GTK_ENTRY (_glade->GetWidget ("teamsize_entry"));

        if (w)
        {
          gchar *value = (gchar *) gtk_entry_get_text (w);

          if (value)
          {
            _minimum_team_size->_value = atoi (value);
          }
        }
      }

      if (team)
      {
        team->SetDefaultClassification (_default_classification->_value);
        team->SetMinimumSize           (_minimum_team_size->_value);
        team->SetManualClassification  (_manual_classification->_value);
        Update (team);
      }
      else
      {
        GSList *current = _player_list;

        while (current)
        {
          Player *player = (Player *) current->data;

          if (player->Is ("Team"))
          {
            Team *current_team = (Team *) player;

            current_team->SetDefaultClassification (_default_classification->_value);
            current_team->SetMinimumSize           (_minimum_team_size->_value);
            current_team->SetManualClassification  (_manual_classification->_value);
            Update (current_team);
          }
          current = g_slist_next (current);
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::SetPasteVisibility (gboolean visibility)
  {
    GtkAction *paste_action = GetAction ("PasteAction");

    if (paste_action)
    {
      gtk_action_set_visible (paste_action,
                              visibility);
    }
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::EnableDragAndDrop ()
  {
    if (_contest->IsTeamEvent ())
    {
      gtk_tree_view_enable_model_drag_source (_tree_view,
                                              GDK_BUTTON1_MASK,
                                              _dnd_config->GetTargetTable (),
                                              _dnd_config->GetTargetTableSize (),
                                              GDK_ACTION_MOVE);
      gtk_tree_view_enable_model_drag_dest (_tree_view,
                                            _dnd_config->GetTargetTable (),
                                            _dnd_config->GetTargetTableSize (),
                                            GDK_ACTION_MOVE);
    }
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::DisableDragAndDrop ()
  {
    gtk_tree_view_unset_rows_drag_source (_tree_view);
    gtk_tree_view_unset_rows_drag_dest   (_tree_view);
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::SetTeamEvent (gboolean team_event)
  {
    if (team_event)
    {
      SelectTreeMode ();
      _form->ShowPage ("Team");
      gtk_widget_show (_glade->GetWidget ("team_classification_label"));
      gtk_widget_show (_glade->GetWidget ("team_classification_viewport"));
      gtk_widget_show (_glade->GetWidget ("teamsize_label"));
      gtk_widget_show (_glade->GetWidget ("teamsize_viewport"));
      gtk_widget_show (_glade->GetWidget ("tree_control_hbox"));

      if (Locked () == FALSE)
      {
        EnableDragAndDrop ();
      }
    }
    else
    {
      SelectFlatMode ();
      _form->HidePage ("Team");
      gtk_widget_hide (_glade->GetWidget ("team_classification_label"));
      gtk_widget_hide (_glade->GetWidget ("team_classification_viewport"));
      gtk_widget_hide (_glade->GetWidget ("teamsize_label"));
      gtk_widget_hide (_glade->GetWidget ("teamsize_viewport"));
      gtk_widget_hide (_glade->GetWidget ("tree_control_hbox"));

      DisableDragAndDrop ();
    }

    // On loading, sensitivity is set by default to TRUE.
    if (Locked ())
    {
      SetSensitiveState (FALSE);
      SetPasteVisibility (FALSE);
    }
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::OnLoadingCompleted ()
  {
    GSList *current = _checksum_list;

    for (guint ref = 1; current; ref++)
    {
      Player *player = (Player *) current->data;

      {
        player->SetRef (ref);
        Update (player);
      }

      current = g_slist_next (current);
    }
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::OnLocked ()
  {
    DisableSensitiveWidgets ();
    SetSensitiveState (FALSE);
    SetPasteVisibility (FALSE);

    UpdateChecksum ();
    UpdateRanking  ();

    _form->Lock ();

    DisableDragAndDrop ();
  }

  // --------------------------------------------------------------------------------
  gboolean CheckinSupervisor::PresentPlayerFilter (Player      *player,
                                                   PlayersList *owner)
  {
    if (Checkin::PresentPlayerFilter (player, owner))
    {
      CheckinSupervisor *supervisor = dynamic_cast <CheckinSupervisor *> (owner);

      if (player->Is ("team"))
      {
        return (supervisor->_contest->IsTeamEvent () == TRUE);
      }
      else
      {
        return (supervisor->_contest->IsTeamEvent () == FALSE);
      }
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  GSList *CheckinSupervisor::GetCurrentClassification ()
  {
    GSList *result = CreateCustomList (PresentPlayerFilter, this);

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
    SetPasteVisibility (TRUE);
    _form->UnLock ();
    EnableDragAndDrop ();
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::RegisterNewTeam (Team *team)
  {
    AttributeDesc *team_desc = AttributeDesc::GetDescFromCodeName ("team");
    gchar         *name      = team->GetName ();

    if (team_desc->GetXmlImage (name) == NULL)
    {
      team_desc->AddDiscreteValues (name,
                                    name,
                                    NULL,
                                    NULL);
    }

    ApplyConfig (team);
  }

  // --------------------------------------------------------------------------------
  Team *CheckinSupervisor::GetTeam (const gchar *name)
  {
    Player              *team;
    Player::AttributeId  name_attr_id ("name");
    Attribute           *name_attr = Attribute::New ("name");

    name_attr->SetValue (name);

    team = GetPlayerWithAttribute (&name_attr_id,
                                   name_attr);
    name_attr->Release ();

    if (team && team->Is ("Team"))
    {
      return (Team *) team;
    }
    else
    {
      return NULL;
    }
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::OnPlayerRemoved (Player *player)
  {
    if (player->Is ("Team"))
    {
      Team   *team    = (Team *) player;
      GSList *members = g_slist_copy (team->GetMemberList ());
      GSList *current = members;

      while (current)
      {
        Player *player = (Player *) current->data;

        Remove (player);
        current = g_slist_next (current);
      }
      g_slist_free (members);
    }
    else if (player->Is ("Fencer"))
    {
      Fencer *fencer = (Fencer *) player;
      Team   *team   = fencer->GetTeam ();

      if (team)
      {
        fencer->SetTeam (_null_team);
        team->SetAttendingFromMembers ();
        Update (team);
      }
    }

    Checkin::OnPlayerRemoved (player);
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::Monitor (Player *player)
  {
    Checkin::Monitor (player);

    if (player->Is ("Team"))
    {
      player->SetChangeCbk ("attending",
                            (Player::OnChange) OnAttrAttendingChanged,
                            this);
      player->SetChangeCbk ("name",
                            (Player::OnChange) OnAttrTeamRenamed,
                            this,
                            Player::AFTER_CHANGE);
    }

    if (player->Is ("Fencer"))
    {
      player->SetChangeCbk ("attending",
                            (Player::OnChange) OnAttrAttendingChanged,
                            this);
      player->SetChangeCbk ("ranking",
                            (Player::OnChange) OnAttrAttendingChanged,
                            this);
      player->SetChangeCbk ("team",
                            (Player::OnChange) OnAttrTeamChanged,
                            this,
                            Player::BEFORE_CHANGE | Player::AFTER_CHANGE);
      player->NotifyChange ("team");
    }
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::TogglePlayerAttr (Player              *player,
                                            Player::AttributeId *attr_id,
                                            gboolean             new_value)
  {
    // Teams can't be toggled
    if (player && (player->Is ("Team") == FALSE))
    {
      Checkin::TogglePlayerAttr (player,
                                 attr_id,
                                 new_value);
    }
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::OnAttrAttendingChanged (Player    *player,
                                                  Attribute *attr,
                                                  Object    *owner,
                                                  guint      step)
  {
    CheckinSupervisor *supervisor = dynamic_cast <CheckinSupervisor *> (owner);

    if (player->Is ("Fencer"))
    {
      Fencer *fencer = (Fencer *) player;
      Team   *team   = fencer->GetTeam ();

      if (team)
      {
        team->SetAttendingFromMembers ();
        supervisor->Update (team);
      }
    }
    else if (player->Is ("Team"))
    {
      supervisor->Update (player);
    }
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::OnAttrTeamRenamed (Player    *player,
                                             Attribute *attr,
                                             Object    *owner,
                                             guint      step)
  {
    CheckinSupervisor *supervisor = dynamic_cast <CheckinSupervisor *> (owner);
    Team              *team       = (Team *) player;

    {
      Player::AttributeId  team_attr_id ("team");
      GSList *member_list = g_slist_copy (team->GetMemberList ());
      GSList *current     = member_list;

      while (current)
      {
        Player *member = (Player *) current->data;

        member->SetAttributeValue (&team_attr_id,
                                   attr->GetStrValue ());

        current = g_slist_next (current);
      }

      g_slist_free (member_list);
    }

    supervisor->RegisterNewTeam (team);
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::OnAttrTeamChanged (Player    *player,
                                             Attribute *attr,
                                             Object    *owner,
                                             guint      step)
  {
    CheckinSupervisor   *supervisor = dynamic_cast <CheckinSupervisor *> (owner);
    Player::AttributeId  attending_attr_id ("attending");

    if (step == Player::BEFORE_CHANGE)
    {
      {
        Attribute *attending_attr = player->GetAttribute (&attending_attr_id);

        // To refresh the status of the old team
        // attending is temporarily set to 0
        // to be restored on AFTER_CHANGE step when
        // the new team is really set.
        player->SetData (owner,
                         "attending_to_restore",
                         (void *) attending_attr->GetUIntValue ());

        player->SetAttributeValue (&attending_attr_id,
                                   (guint) 0);
      }

      player->SetData (owner,
                       "previous_team",
                       g_strdup (attr->GetStrValue ()),
                       g_free);
    }
    else
    {
      {
        gchar  *team_name = attr->GetStrValue ();
        Fencer *fencer    = (Fencer *) player;
        Team   *team      = supervisor->_null_team;

        // Find a team with the given name
        if (team_name && team_name[0])
        {
          team = supervisor->GetTeam (team_name);

          // Create a team if necessary
          if (team == NULL)
          {
            team = (Team *) PlayerFactory::CreatePlayer ("Team");

            team->SetName (team_name);
            supervisor->RegisterNewTeam (team);
            supervisor->Add (team);
          }
        }

        {
          fencer->SetTeam (team);
          team->SetAttributesFromMembers ();
          supervisor->UpdateHierarchy (player);
          supervisor->Update (team);
        }

        {
          Team *previous_team = supervisor->GetTeam ((gchar *) player->GetPtrData (owner,
                                                                                   "previous_team"));

          if (previous_team)
          {
            previous_team->SetAttributesFromMembers ();
            supervisor->Update (previous_team);
            player->RemoveData (owner,
                                "previous_team");
          }
        }
      }

      player->SetAttributeValue (&attending_attr_id,
                                 player->GetUIntData (owner,
                                                      "attending_to_restore"));
      player->RemoveData (owner,
                          "attending_to_restore");
      supervisor->Update (player);
    }
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::OnPlayerEventFromForm (Player            *player,
                                                 Form::PlayerEvent  event)
  {
    if (event == Form::NEW_PLAYER)
    {
      if (player->Is ("Team"))
      {
        if (GetTeam (player->GetName ()))
        {
          return;
        }

        RegisterNewTeam ((Team *) player);
      }
    }

    Checkin::OnPlayerEventFromForm (player,
                                    event);

    if (player->Is ("Fencer"))
    {
      Fencer *fencer = (Fencer *) player;
      Team   *team   = fencer->GetTeam ();

      if (team)
      {
        team->SetAttributesFromMembers ();
        Update (team);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::OnImportRanking ()
  {
    RankImporter *importer = new RankImporter (_config_file);

    {
      GSList *current = _player_list;

      while (current)
      {
        Player *fencer = (Player *) current->data;

        importer->ModifyRank (fencer);
        Update (fencer);

        current = g_slist_next (current);
      }
    }

    importer->Release ();

    OnListChanged ();
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::OnManualRadioButtonToggled (GtkToggleButton *button)
  {
    if (gtk_toggle_button_get_active (button))
    {
      gtk_widget_hide (GTK_WIDGET (_glade->GetWidget ("derived_table")));
    }
    else
    {
      gtk_widget_show         (GTK_WIDGET (_glade->GetWidget ("derived_table")));
      gtk_widget_queue_resize (GTK_WIDGET (_glade->GetWidget ("team_table")));
    }
  }

  // --------------------------------------------------------------------------------
  Player *CheckinSupervisor::GetFencerFromDndRef (guint ref)
  {
    GSList *current = _player_list;

    while (current)
    {
      Player *player = (Player *) current->data;

      if (player->GetDndRef () == ref)
      {
        return player;
      }
      current = g_slist_next (current);
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::OnDragDataGet (GtkWidget        *widget,
                                         GdkDragContext   *drag_context,
                                         GtkSelectionData *selection_data,
                                         guint             target_type,
                                         guint             time)
  {

    if (target_type == _dnd_target)
    {
      GSList  *selected   = GetSelectedPlayers ();
      Player  *fencer     = (Player *) selected->data;
      guint32  fencer_ref = fencer->GetDndRef ();

      gtk_selection_data_set (selection_data,
                              gtk_selection_data_get_target (selection_data),
                              32,
                              (guchar *) &fencer_ref,
                              sizeof (fencer_ref));
    }
  }

  // --------------------------------------------------------------------------------
  gboolean CheckinSupervisor::OnDragMotion (GtkWidget      *widget,
                                            GdkDragContext *drag_context,
                                            gint            x,
                                            gint            y,
                                            guint           time)
  {
    Player *player;

    Module::OnDragMotion (widget,
                          drag_context,
                          x,
                          y,
                          time);

    player = (Player *) _dnd_config->GetFloatingObject ();

    if (player->Is ("Fencer"))
    {
      GtkTreePath             *path;
      GtkTreeViewDropPosition  pos;

      if (gtk_tree_view_get_dest_row_at_pos (_tree_view,
                                             x,
                                             y,
                                             &path,
                                             &pos))
      {
        gtk_tree_view_set_drag_dest_row (_tree_view,
                                         path,
                                         pos);
        return FALSE;
      }
    }

    gdk_drag_status  (drag_context,
                      GDK_ACTION_PRIVATE,
                      time);

    return TRUE;
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::OnDragDataReceived (GtkWidget        *widget,
                                              GdkDragContext   *drag_context,
                                              gint              x,
                                              gint              y,
                                              GtkSelectionData *selection_data,
                                              guint             target_type,
                                              guint             time)
  {
    if (target_type == _dnd_target)
    {
      if (selection_data && (gtk_selection_data_get_length (selection_data) >= 0))
      {
        guint32 *toto   = (guint32 *) gtk_selection_data_get_data (selection_data);
        Player  *fencer = GetFencerFromDndRef (*toto);

        _dnd_config->SetFloatingObject (fencer);
      }
    }
  }

  // --------------------------------------------------------------------------------
  gboolean CheckinSupervisor::OnDragDrop (GtkWidget      *widget,
                                          GdkDragContext *drag_context,
                                          gint            x,
                                          gint            y,
                                          guint           time)
  {
    gboolean  result = FALSE;
    Player   *player = (Player *) _dnd_config->GetFloatingObject ();

    if (player->Is ("Fencer"))
    {
      Fencer                  *fencer = (Fencer *) player;
      GtkTreePath             *path;
      GtkTreeViewDropPosition  pos;

      if (gtk_tree_view_get_dest_row_at_pos (_tree_view,
                                             x,
                                             y,
                                             &path,
                                             &pos))
      {
        GtkTreeModel        *model = gtk_tree_view_get_model (_tree_view);
        GtkTreeIter          iter;
        Player              *dest;
        Player::AttributeId  attr_id ("team");

        gtk_tree_model_get_iter (model,
                                 &iter,
                                 path);

        gtk_tree_model_get (model, &iter,
                            gtk_tree_model_get_n_columns (model) - 1,
                            &dest, -1);

        if (dest->Is ("Fencer"))
        {
          Fencer *fencer = (Fencer *) dest;

          dest = fencer->GetTeam ();
        }

        fencer->SetAttributeValue (&attr_id,
                                   dest->GetName ());
        result = TRUE;
      }
    }

    gtk_drag_finish (drag_context,
                     result,
                     FALSE,
                     time);

    return result;
  }

  // --------------------------------------------------------------------------------
  void CheckinSupervisor::CellDataFunc (GtkTreeViewColumn *tree_column,
                                        GtkCellRenderer   *cell,
                                        GtkTreeModel      *tree_model,
                                        GtkTreeIter       *iter)
  {
    if (_contest->IsTeamEvent ())
    {
      GtkTreeIter parent_iter;

      if (gtk_tree_model_iter_parent (gtk_tree_view_get_model (_tree_view),
                                      &parent_iter,
                                      iter) == FALSE)
      {
        g_object_set (cell,
                      "weight", PANGO_WEIGHT_BOLD,
                      NULL);
      }
      else
      {
        g_object_set (cell,
                      "weight", PANGO_WEIGHT_NORMAL,
                      NULL);
      }
    }
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT gboolean on_expand_eventbox_button_press_event (GtkWidget *widget,
                                                                             GdkEvent  *event,
                                                                             Object    *owner)
  {
    CheckinSupervisor *supervisor = dynamic_cast <CheckinSupervisor *> (owner);

    supervisor->ExpandAll ();
    return TRUE;
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT gboolean on_collapse_eventbox_button_press_event (GtkWidget *widget,
                                                                               GdkEvent  *event,
                                                                               Object    *owner)
  {
    CheckinSupervisor *supervisor = dynamic_cast <CheckinSupervisor *> (owner);

    supervisor->CollapseAll ();
    return TRUE;
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_manual_radiobutton_toggled (GtkWidget *widget,
                                                                 Object    *owner)
  {
    CheckinSupervisor *supervisor = dynamic_cast <CheckinSupervisor *> (owner);

    supervisor->OnManualRadioButtonToggled (GTK_TOGGLE_BUTTON (widget));
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_ranking_toolbutton_clicked (GtkWidget *widget,
                                                                 Object    *owner)
  {
    CheckinSupervisor *supervisor = dynamic_cast <CheckinSupervisor *> (owner);

    supervisor->OnImportRanking ();
  }
}