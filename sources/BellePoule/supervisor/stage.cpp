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

#include <stdlib.h>

#include "classification.hpp"
#include "contest.hpp"
#include "util/player.hpp"
#include "util/canvas.hpp"

#include "stage.hpp"

GSList *Stage::_stage_base = NULL;

// --------------------------------------------------------------------------------
Stage::Stage (StageClass *stage_class)
{
  _name                  = g_strdup ("");
  _locked                = FALSE;
  _result                = NULL;
  _output_short_list     = NULL;
  _quota_exceedance      = 0;
  _previous              = NULL;
  _next                  = NULL;
  _stage_class           = stage_class;
  _attendees             = NULL;
  _classification        = NULL;
  _input_provider        = NULL;
  _classification_on     = FALSE;

  _score_stuffing_trigger = NULL;

  _status_listener = NULL;

  _max_score = NULL;

  _nb_qualified = new Data ("NbQualifiesParIndice",
                            (guint) 0);
  DeactivateNbQualified ();
}

// --------------------------------------------------------------------------------
Stage::~Stage ()
{
  FreeResult ();
  g_free (_name);

  TryToRelease (_score_stuffing_trigger);
  TryToRelease (_classification);
  TryToRelease (_attendees);
  TryToRelease (_nb_qualified);

  if (_previous)
  {
    _previous->_next = NULL;
  }
}

// --------------------------------------------------------------------------------
Stage::StageView Stage::GetStageView (GtkPrintOperation *operation)
{
  return (StageView) GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (operation),
                                                          "PRINT_STAGE_VIEW"));
}

// --------------------------------------------------------------------------------
void Stage::SetStatusListener (Listener *listener)
{
  _status_listener = listener;
}

// --------------------------------------------------------------------------------
void Stage::SignalStatusUpdate ()
{
  if (_status_listener)
  {
    _status_listener->OnStageStatusChanged (this);
  }
}

// --------------------------------------------------------------------------------
const gchar *Stage::GetKlassName ()
{
  return _class->_name;
}

// --------------------------------------------------------------------------------
gchar *Stage::GetName ()
{
  if (_input_provider)
  {
    return _input_provider->_name;
  }
  else
  {
    return _name;
  }
}

// --------------------------------------------------------------------------------
gchar *Stage::GetFullName ()
{
  return g_strdup_printf ("%s\n%s", _class->_name, GetName ());
}

// --------------------------------------------------------------------------------
void Stage::SetName (gchar *name)
{
  if (name)
  {
    if (_input_provider)
    {
      _input_provider->_name = g_strdup (name);
    }
    else
    {
      g_free (_name);
      _name = g_strdup (name);
    }
  }
}

// --------------------------------------------------------------------------------
guint32 Stage::GetRandSeed ()
{
  Module *module = dynamic_cast <Module *> (this);

  if (module)
  {
    return module->_rand_seed;
  }

  return 0;
}

// --------------------------------------------------------------------------------
void Stage::SetRandSeed (guint32 rand_seed)
{
  Module *module = dynamic_cast <Module *> (this);

  if (module)
  {
    module->_rand_seed = rand_seed;
  }

  if (_previous)
  {
    _previous->GiveEliminatedAFinalRank ();
  }
}

// --------------------------------------------------------------------------------
void Stage::SetId (guint id)
{
  _id = id;
}

// --------------------------------------------------------------------------------
guint Stage::GetId ()
{
  return _id;
}

// --------------------------------------------------------------------------------
void Stage::FreeResult ()
{
  g_slist_free (_result);
  _result = NULL;

  g_slist_free (_output_short_list);
  _output_short_list = NULL;
  _quota_exceedance  = 0;
}

// --------------------------------------------------------------------------------
void Stage::Reset ()
{
  if (_attendees)
  {
    GSList *current = _attendees->GetShortList ();
    Player::AttributeId status_attr_id        ("status", GetPlayerDataOwner ());
    Player::AttributeId global_status_attr_id ("global_status");
    Player::AttributeId final_rank_attr_id    ("final_rank");

    while (current)
    {
      Player *player = (Player *) current->data;

      player->SetAttributeValue (&status_attr_id,
                                 "Q");
      player->SetAttributeValue (&global_status_attr_id,
                                 "Q");

      player->RemoveAttribute (&final_rank_attr_id);

      current = g_slist_next (current);
    }
  }
}

// --------------------------------------------------------------------------------
void Stage::Display ()
{
  Spread ();
}

// --------------------------------------------------------------------------------
void Stage::Lock ()
{
  _locked = TRUE;
  OnLocked ();
  SetResult ();
  Spread ();
}

// --------------------------------------------------------------------------------
void Stage::UnLock ()
{
  FreeResult ();

  if (_next && _next->_attendees)
  {
    _next->_attendees->Release ();
    _next->_attendees = NULL;
  }

  _locked = FALSE;
  OnUnLocked ();
  Spread ();
}

// --------------------------------------------------------------------------------
gboolean Stage::Locked ()
{
  return _locked;
}

// --------------------------------------------------------------------------------
gboolean Stage::IsOver ()
{
  return TRUE;
}

// --------------------------------------------------------------------------------
gchar *Stage::GetError ()
{
  return NULL;
}

// --------------------------------------------------------------------------------
Data *Stage::GetMaxScore ()
{
  return _max_score;
}

// --------------------------------------------------------------------------------
void Stage::ApplyConfig ()
{
  Module *module = dynamic_cast <Module *> (this);

  if (module)
  {
    {
      GtkEntry *w = GTK_ENTRY (module->GetGObject ("name_entry"));

      if (w)
      {
        gchar *name = (gchar *) gtk_entry_get_text (w);

        SetName (name);
      }
    }

    {
      GtkEntry *w = GTK_ENTRY (module->GetGObject ("max_score_entry"));

      if (w)
      {
        gchar *str = (gchar *) gtk_entry_get_text (w);

        if (str)
        {
          _max_score->_value = atoi (str);
        }
      }
    }

    {
      GtkSpinButton *w = GTK_SPIN_BUTTON (module->GetGObject ("nb_qualified_spinbutton"));

      if (w)
      {
        _nb_qualified->_value = gtk_spin_button_get_value_as_int (w);
      }
    }
  }
}

// --------------------------------------------------------------------------------
void Stage::FillInConfig ()
{
  Module *module = dynamic_cast <Module *> (this);

  if (module)
  {
    {
      GtkEntry *w = GTK_ENTRY (module->GetGObject ("name_entry"));

      if (w)
      {
        gtk_entry_set_text (w,
                            GetName ());
      }
    }

    {
      GtkEntry *w = GTK_ENTRY (module->GetGObject ("max_score_entry"));

      if (w)
      {
        gchar *text = g_strdup_printf ("%d", _max_score->_value);

        gtk_entry_set_text (w,
                            text);
        g_free (text);
      }
    }

    {
      GtkSpinButton *w = GTK_SPIN_BUTTON (module->GetGObject ("nb_qualified_spinbutton"));

      if (w)
      {
        gtk_spin_button_set_value (w,
                                   _nb_qualified->_value);
      }
    }
  }
}

// --------------------------------------------------------------------------------
void Stage::SetPrevious (Stage *previous)
{
  _previous = previous;
  if (_previous)
  {
    _previous->_next = this;
  }
}

// --------------------------------------------------------------------------------
Stage *Stage::GetPreviousStage ()
{
  return _previous;
}

// --------------------------------------------------------------------------------
Stage *Stage::GetNextStage ()
{
  return _next;
}

// --------------------------------------------------------------------------------
void Stage::RetrieveAttendees ()
{
  if (_attendees)
  {
    _attendees->Release ();
  }

  if (_previous)
  {
    GSList *shortlist = _previous->_output_short_list;

    _attendees = new Attendees (_previous->_attendees,
                                _previous->_output_short_list);

    // Status
    {
      Player::AttributeId  stage_start_rank_attr_id ("stage_start_rank", GetPlayerDataOwner ());
      Player::AttributeId  status_attr_id ("status", GetPlayerDataOwner ());
      Player::AttributeId  global_status_attr_id ("global_status");
      GSList              *current = shortlist;

      for (guint i = 0; current != NULL; i++)
      {
        Player *player = (Player *) current->data;

        player->SetAttributeValue (&stage_start_rank_attr_id,
                                   i+1);
        player->SetAttributeValue (&status_attr_id,
                                   "Q");
        player->SetAttributeValue (&global_status_attr_id,
                                   "Q");

        current = g_slist_next (current);
      }
    }

    if (_nb_qualified->IsValid () == FALSE)
    {
      _nb_qualified->_value = g_slist_length (shortlist);
    }
  }
  else
  {
    _attendees = new Attendees ();
  }
}

// --------------------------------------------------------------------------------
guint Stage::GetQuotaExceedance ()
{
  return _quota_exceedance;
}

// --------------------------------------------------------------------------------
void Stage::SetOutputShortlist ()
{
  if (_output_short_list)
  {
    g_slist_free (_output_short_list);
    _output_short_list = NULL;
  }

  if (_result)
  {
    _output_short_list = g_slist_copy (_result);
  }

  if (_output_short_list && _classification)
  {
    Player::AttributeId classif_attr_id       ("status", GetPlayerDataOwner ());
    Player::AttributeId global_status_attr_id ("global_status");

    // Reset status from N to Q before applying Quota
    {
      GSList *current = _output_short_list;

      while (current)
      {
        Player    *player = (Player *) current->data;
        Attribute *status;

        status = player->GetAttribute (&classif_attr_id);
        if (status)
        {
          gchar *value = status->GetStrValue ();

          if (value && (value[0] == 'N'))
          {
            player->SetAttributeValue (&classif_attr_id,
                                       "Q");
          }
        }

        status = player->GetAttribute (&global_status_attr_id);
        if (status)
        {
          gchar *value = status->GetStrValue ();

          if (value && (value[0] == 'N'))
          {
            player->SetAttributeValue (&global_status_attr_id,
                                       "Q");
          }
        }

        current = g_slist_next (current);
      }
    }

    // Remove all of the withdrawalls and black cards
    {
      Player::AttributeId stage_status_attr_id ("status", GetPlayerDataOwner ());
      GSList *new_short_list = NULL;
      GSList *current        = _output_short_list;

      while (current)
      {
        Player    *player            = (Player *) current->data;
        Attribute *stage_status_attr = player->GetAttribute (&stage_status_attr_id);

        if (stage_status_attr)
        {
          gchar *value = stage_status_attr->GetStrValue ();

          if (value
              && (value[0] != 'Q') && value[0] != 'N')
          {
            player->SetAttributeValue (&global_status_attr_id,
                                       value);
          }
          else
          {
            new_short_list = g_slist_append (new_short_list, player);
          }
        }

        current = g_slist_next (current);
      }

      g_slist_free (_output_short_list);
      _output_short_list = new_short_list;
    }

    // Remove all of the fencers not promoted in the barrage round
    {
      Player::AttributeId promoted_attr_id ("promoted", GetPlayerDataOwner ());
      GSList *current = g_slist_last (_output_short_list);

      while (current)
      {
        Player    *player   = (Player *) current->data;
        Attribute *promoted = player->GetAttribute (&promoted_attr_id);

        if (promoted && (promoted->GetUIntValue () == FALSE))
        {
          player->SetAttributeValue (&classif_attr_id,
                                     "N");
          player->SetAttributeValue (&global_status_attr_id,
                                     "N");

          _output_short_list = g_slist_delete_link (_output_short_list,
                                                    current);
        }
        else
        {
          break;
        }

        current = g_slist_last (_output_short_list);
      }
    }

    // Quota
    if (_nb_qualified->IsValid () && (_nb_qualified->_value > 0))
    {
      Player *last_qualified = (Player *) g_slist_nth_data (_output_short_list, _nb_qualified->_value-1);

      if (last_qualified)
      {
        Player::AttributeId  rank_attr_id ("rank", this);
        Attribute           *last_qualified_rank = last_qualified->GetAttribute (&rank_attr_id);
        GSList              *current             = g_slist_last (_output_short_list);

        while (current)
        {
          Player    *player = (Player *) current->data;
          Attribute *rank   = player->GetAttribute (&rank_attr_id);

          if (rank->GetUIntValue () <= last_qualified_rank->GetUIntValue ())
          {
            break;
          }

          {
            Player::AttributeId stage_attr_id ("status", GetPlayerDataOwner ());
            Attribute *stage_status_attr = player->GetAttribute (&stage_attr_id);

            if (stage_status_attr)
            {
              gchar *value = stage_status_attr->GetStrValue ();

              if (value && (value[0] == 'Q'))
              {
                player->SetAttributeValue (&classif_attr_id,
                                           "N");
                player->SetAttributeValue (&global_status_attr_id,
                                           "N");
              }
            }
          }

          _output_short_list = g_slist_delete_link (_output_short_list,
                                                    current);
          current = g_slist_last (_output_short_list);
        }
      }

      // Quota exceedance
      if (g_slist_length (_output_short_list) > _nb_qualified->_value)
      {
        GSList *reversed_short_list = g_slist_copy (_output_short_list);

        reversed_short_list = g_slist_reverse (reversed_short_list);
        {
          GSList              *current = reversed_short_list;
          Player::AttributeId  rank_attr_id ("rank", this);
          Attribute           *last_qualified_rank = NULL;

          _quota_exceedance = g_slist_length (_output_short_list);
          for (guint i = 0; current != NULL; i++)
          {
            Player    *current_fencer = (Player *) current->data;
            Attribute *current_rank   = current_fencer->GetAttribute (&rank_attr_id);

            if (last_qualified_rank == NULL)
            {
              last_qualified_rank = current_rank;
            }
            else if (current_rank->GetUIntValue () < last_qualified_rank->GetUIntValue ())
            {
              _quota_exceedance = i;
              break;
            }

            current = g_slist_next (current);
          }
        }
        g_slist_free (reversed_short_list);
      }
    }
  }
}

// --------------------------------------------------------------------------------
void Stage::LoadAttendees (xmlNode *n)
{
  if (_attendees == NULL)
  {
    if (_previous)
    {
      _attendees = new Attendees (_previous->_attendees,
                                  _previous->_output_short_list);
    }
    else
    {
      _attendees = new Attendees ();
    }

    if (_nb_qualified->IsValid () == FALSE)
    {
      _nb_qualified->_value = g_slist_length (_attendees->GetShortList ());
    }
  }

  if (n)
  {
    gchar *ref_attr = (gchar *) xmlGetProp (n, BAD_CAST "REF");

    if (ref_attr)
    {
      Player *player = GetFencerFromRef (atoi (ref_attr));

      if (player)
      {
        {
          Player::AttributeId attr_id ("stage_start_rank", this);
          gchar *rank_attr =  (gchar *) xmlGetProp (n, BAD_CAST "RangInitial");

          if (rank_attr)
          {
            player->SetAttributeValue (&attr_id,
                                       atoi (rank_attr));
            xmlFree (rank_attr);
          }
          else
          {
            player->SetAttributeValue (&attr_id,
                                       (guint) 0);
          }
        }

        {
          Player::AttributeId status_attr_id        ("status", GetPlayerDataOwner ());
          Player::AttributeId global_status_attr_id ("global_status");
          gchar *status_attr =  (gchar *) xmlGetProp (n, BAD_CAST "Statut");

          if (status_attr)
          {
            player->SetAttributeValue (&status_attr_id,
                                       status_attr);
            player->SetAttributeValue (&global_status_attr_id,
                                       status_attr);
            xmlFree (status_attr);
          }
          else
          {
            player->SetAttributeValue (&status_attr_id,
                                       "Q");
            player->SetAttributeValue (&global_status_attr_id,
                                       "Q");
          }
        }
      }

      xmlFree (ref_attr);
    }
  }
}

// --------------------------------------------------------------------------------
void Stage::ActivateNbQualified ()
{
  _nb_qualified->Activate ();

  {
    Module *module;

    if (GetInputProviderClient ())
    {
      module = dynamic_cast <Module *> (_next);
    }
    else
    {
      module = dynamic_cast <Module *> (this);
    }

    if (module)
    {
      gtk_widget_show         (GTK_WIDGET (module->GetGObject ("nb_qualified_spinbutton")));
      gtk_widget_queue_resize (GTK_WIDGET (module->GetGObject ("qualified_viewport")));
    }
  }
}

// --------------------------------------------------------------------------------
void Stage::DeactivateNbQualified ()
{
  _nb_qualified->Deactivate ();

  {
    Module *module;

    if (GetInputProviderClient ())
    {
      module = dynamic_cast <Module *> (_next);
    }
    else
    {
      module = dynamic_cast <Module *> (this);
    }

    if (module)
    {
      gtk_widget_hide (GTK_WIDGET (module->GetGObject ("nb_qualified_spinbutton")));
    }
  }
}

// --------------------------------------------------------------------------------
Player *Stage::GetFencerFromRef (guint ref)
{
  GSList *current = _attendees->GetShortList ();

  while (current)
  {
    Player *player = (Player *) current->data;

    if (player->GetRef () == ref)
    {
      return player;
    }
    current = g_slist_next (current);
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void Stage::RegisterStageClass (const gchar *name,
                                const gchar *xml_name,
                                Creator      creator,
                                guint        rights)
{
  StageClass *stage_class = new StageClass;

  stage_class->_name     = name;
  stage_class->_xml_name = xml_name;
  stage_class->_creator  = creator;
  stage_class->_rights   = (Rights) rights;

  _stage_base = g_slist_append (_stage_base,
                                (void *) stage_class);
}

// --------------------------------------------------------------------------------
guint Stage::GetNbStageClass ()
{
  return g_slist_length (_stage_base);
}

// --------------------------------------------------------------------------------
Stage::StageClass *Stage::GetClass (guint index)
{
  return (StageClass *) g_slist_nth_data (_stage_base,
                                          index);
}

// --------------------------------------------------------------------------------
Stage::StageClass *Stage::GetClass ()
{
  return _class;
}

// --------------------------------------------------------------------------------
Stage::StageClass *Stage::GetClass (const gchar *name)
{
  if (name)
  {
    GSList *current = _stage_base;

    while (current)
    {
      StageClass *stage_class = (StageClass *) current->data;

      if (g_strcmp0 (name, stage_class->_xml_name) == 0)
      {
        return stage_class;
      }
      current = g_slist_next (current);
    }
  }
  return NULL;
}

// --------------------------------------------------------------------------------
void Stage::SetContest (Contest *contest)
{
  _contest = contest;
}

// --------------------------------------------------------------------------------
Contest *Stage::GetContest ()
{
  return _contest;
}

// --------------------------------------------------------------------------------
Stage *Stage::CreateInstance (xmlNode *xml_node)
{
  if (xml_node)
  {
    return CreateInstance ((gchar *) xml_node->name);
  }

  return NULL;
}

// --------------------------------------------------------------------------------
Stage *Stage::CreateInstance (const gchar *name)
{
  StageClass *stage_class = GetClass (name);

  if (stage_class)
  {
    Stage *stage = stage_class->_creator (stage_class);

    if (stage)
    {
      stage->_class = stage_class;
    }
    return stage;
  }

  return NULL;
}

// --------------------------------------------------------------------------------
guint Stage::GetRights ()
{
  return _stage_class->_rights;
}

// --------------------------------------------------------------------------------
const gchar *Stage::GetInputProviderClient ()
{
  return NULL;
}

// --------------------------------------------------------------------------------
void Stage::InitQualifiedForm ()
{
  Module *module = dynamic_cast <Module *> (this);

  // Let's force the configuration form
  // to be set properly
  if (module->GetGObject ("number_radiobutton"))
  {
    if (_nb_qualified->IsValid ())
    {
      ActivateNbQualified ();
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (module->GetGObject ("number_radiobutton")),
                                    TRUE);
    }
    else
    {
      DeactivateNbQualified ();
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (module->GetGObject ("all_radiobutton")),
                                    TRUE);
    }
  }
}

// --------------------------------------------------------------------------------
void Stage::SetInputProvider (Stage *input_provider)
{
  _input_provider = input_provider;

  TryToRelease (_nb_qualified);
  _nb_qualified = input_provider->_nb_qualified;
  _nb_qualified->Retain ();
}

// --------------------------------------------------------------------------------
Stage *Stage::GetInputProvider ()
{
  return _input_provider;
}

// --------------------------------------------------------------------------------
void Stage::OnFilterClicked (const gchar *classification_toggle_button)
{
  Module              *module = dynamic_cast <Module *> (this);
  GtkToggleToolButton *w      = NULL;

  if (classification_toggle_button)
  {
    w = GTK_TOGGLE_TOOL_BUTTON (module->GetGObject (classification_toggle_button));
  }

  if ((w == NULL) || gtk_toggle_tool_button_get_active (w))
  {
    if (_classification)
    {
      _classification->SelectAttributes ();
    }
  }
  else
  {
    module->SelectAttributes ();
  }
}

// --------------------------------------------------------------------------------
void Stage::ToggleClassification (gboolean classification_on)
{
  Module *module = dynamic_cast <Module *> (this);

  if (classification_on && (Locked () == FALSE))
  {
    SetResult ();
  }

  if (module)
  {
    GtkWidget *main_w           = GTK_WIDGET (module->GetGObject ("main_hook"));
    GtkWidget *classification_w = GTK_WIDGET (module->GetGObject ("classification_hook"));

    if (classification_on)
    {
      if (main_w)
      {
        gtk_widget_hide (main_w);
      }
      gtk_widget_show (classification_w);

      if (_classification_on == FALSE)
      {
        _sensitivity_trigger.SwitchOff ();
      }
    }
    else
    {
      gtk_widget_hide (classification_w);
      if (main_w)
      {
        gtk_widget_show_all (main_w);
      }

      if (_classification_on == TRUE)
      {
        _sensitivity_trigger.SwitchOn ();
      }
    }
  }

  _classification_on = classification_on;
}

// --------------------------------------------------------------------------------
Classification *Stage::GetClassification ()
{
  return _classification;
}

// --------------------------------------------------------------------------------
void Stage::SetClassificationFilter (Filter *filter)
{
  if (_classification == NULL)
  {
    Module    *module           = dynamic_cast <Module *> (this);
    GtkWidget *classification_w = GTK_WIDGET (module->GetGObject ("classification_list_host"));

    if (classification_w == NULL)
    {
      classification_w = GTK_WIDGET (module->GetGObject ("classification_hook"));
    }

    if (classification_w)
    {
      _classification = new Classification (filter);
      _classification->SetDataOwner (this);
      module->Plug (_classification,
                    classification_w);
    }
  }
}

// --------------------------------------------------------------------------------
gboolean Stage::HasItsOwnRanking ()
{
  return TRUE;
}

// --------------------------------------------------------------------------------
void Stage::GiveEliminatedAFinalRank ()
{
  if (_result && (GetInputProviderClient () == NULL))
  {
    Player::AttributeId status_attr_id     ("status", GetPlayerDataOwner ());
    Player::AttributeId rank_attr_id       ("rank",   GetPlayerDataOwner ());
    Player::AttributeId final_rank_attr_id ("final_rank");
    guint   qualified_count      = g_slist_length (_output_short_list);
    guint   withdrawal_fix_count = 0;
    GSList *current              = _result;

    while (current)
    {
      Player    *player    = (Player *) current->data;
      Attribute *rank_attr = player->GetAttribute (&rank_attr_id);

      if ((_next == NULL) || (_next->HasItsOwnRanking () == FALSE))
      {
        player->SetAttributeValue (&final_rank_attr_id,
                                   rank_attr->GetUIntValue ());
      }
      else
      {
        Attribute *status_attr = player->GetAttribute (&status_attr_id);

        if (status_attr)
        {
          gchar *status = status_attr->GetStrValue ();

          if (status[0] == 'A')
          {
            if (rank_attr->GetUIntValue () <= qualified_count)
            {
              withdrawal_fix_count++;
              player->SetAttributeValue (&final_rank_attr_id,
                                         qualified_count + withdrawal_fix_count);
            }
            else
            {
              player->SetAttributeValue (&final_rank_attr_id,
                                         rank_attr->GetUIntValue ());
            }
          }
          else if ((status[0] == 'N') || (status[0] == 'E'))
          {
            player->SetAttributeValue (&final_rank_attr_id,
                                       rank_attr->GetUIntValue ());
          }
        }
      }

      current = g_slist_next (current);
    }
  }
}

// --------------------------------------------------------------------------------
void Stage::UpdateClassification (Classification *classification,
                                  GSList         *result)
{
  if (classification)
  {
    Player::AttributeId *previous_attr_id = NULL;

    if (_input_provider)
    {
      previous_attr_id = new Player::AttributeId ("rank", _previous);
    }

    classification->Wipe ();

    {
      Player::AttributeId *rank_attr_id   = new Player::AttributeId ("rank", this);
      GSList              *current_player = result;

      while (current_player)
      {
        Player *player= (Player *) current_player->data;

        // Rank for input providers
        if (previous_attr_id)
        {
          Attribute *rank_attr = player->GetAttribute (rank_attr_id);

          if (rank_attr)
          {
            player->SetAttributeValue (previous_attr_id,
                                       rank_attr->GetUIntValue ());
          }
        }

        classification->Add (player);

        current_player = g_slist_next (current_player);
      }

      Object::TryToRelease (rank_attr_id);
    }

    Object::TryToRelease (previous_attr_id);
  }
}

// --------------------------------------------------------------------------------
void Stage::SetResult ()
{
  GSList *result = GetCurrentClassification ();

  FreeResult ();
  _result = result;

  SetOutputShortlist ();

  UpdateClassification (_classification,
                        result);
}

// --------------------------------------------------------------------------------
void Stage::LockOnClassification (GtkWidget *w)
{
  _sensitivity_trigger.AddWidget (w);
}

// --------------------------------------------------------------------------------
void Stage::SetScoreStuffingPolicy (gboolean allowed)
{
  if (_score_stuffing_trigger == NULL)
  {
    Module *module = dynamic_cast <Module *> (this);

    if (module)
    {
      _score_stuffing_trigger = new SensitivityTrigger ();

      _score_stuffing_trigger->AddWidget (GTK_WIDGET (module->GetGObject ("stuff_toolbutton")));
    }
  }

  if (allowed)
  {
    _score_stuffing_trigger->SwitchOn ();
  }
  else
  {
    _score_stuffing_trigger->SwitchOff ();
  }
}

// --------------------------------------------------------------------------------
const gchar *Stage::GetXmlPlayerTag ()
{
  if (_contest->IsTeamEvent ())
  {
    return "Equipe";
  }
  else
  {
    return "Tireur";
  }
}

// --------------------------------------------------------------------------------
void Stage::LoadConfiguration (xmlNode *xml_node)
{
  {
    gchar *attr = (gchar *) xmlGetProp (xml_node,
                                        BAD_CAST "ID");

    SetName (attr);
    xmlFree (attr);
  }

  if (_max_score)
  {
    _max_score->Load (xml_node);
  }

  if (_nb_qualified)
  {
    _nb_qualified->Load (xml_node);
  }

  InitQualifiedForm ();
}

// --------------------------------------------------------------------------------
void Stage::SaveConfiguration (xmlTextWriter *xml_writer)
{
  if (GetInputProviderClient ())
  {
    Stage *next_stage = GetNextStage ();

    if (next_stage)
    {
      xmlTextWriterWriteFormatAttribute (xml_writer,
                                         BAD_CAST "PhaseID",
                                         "%d", next_stage->GetId ());
    }
  }
  else
  {
    xmlTextWriterWriteFormatAttribute (xml_writer,
                                       BAD_CAST "PhaseID",
                                       "%d", _id);
  }

  xmlTextWriterWriteAttribute (xml_writer,
                               BAD_CAST "ID",
                               BAD_CAST GetName ());

  if (_max_score)
  {
    _max_score->Save (xml_writer);
  }

  if (_nb_qualified->IsValid ())
  {
    _nb_qualified->Save (xml_writer);
  }
}

// --------------------------------------------------------------------------------
void Stage::Load (xmlNode *n)
{
  LoadAttendees (n);
}

// --------------------------------------------------------------------------------
void Stage::SaveAttendees (xmlTextWriter *xml_writer)
{
  if (_attendees)
  {
    GSList *current = _attendees->GetShortList ();

    while (current)
    {
      Player    *player;
      Attribute *rank;
      Attribute *stage_start_rank;
      Attribute *status;

      player = (Player *) current->data;

      {
        Player::AttributeId *rank_attr_id;
        Player::AttributeId *status_attr_id;
        Player::AttributeId  stage_start_rank_attr_id ("stage_start_rank", this);

        if (GetInputProviderClient ())
        {
          rank_attr_id   = new Player::AttributeId ("rank", _next);
          status_attr_id = new Player::AttributeId ("status", _next);
        }
        else
        {
          rank_attr_id   = new Player::AttributeId ("rank", GetPlayerDataOwner ());
          status_attr_id = new Player::AttributeId ("status", GetPlayerDataOwner ());
        }

        rank             = player->GetAttribute (rank_attr_id);
        stage_start_rank = player->GetAttribute (&stage_start_rank_attr_id);
        status           = player->GetAttribute (status_attr_id);

        rank_attr_id->Release ();
        status_attr_id->Release ();
      }

      xmlTextWriterStartElement (xml_writer,
                                 BAD_CAST player->GetXmlTag ());
      xmlTextWriterWriteFormatAttribute (xml_writer,
                                         BAD_CAST "REF",
                                         "%d", player->GetRef ());
      if (stage_start_rank)
      {
        xmlTextWriterWriteFormatAttribute (xml_writer,
                                           BAD_CAST "RangInitial",
                                           "%d", stage_start_rank->GetUIntValue ());
      }
      if (rank)
      {
        xmlTextWriterWriteFormatAttribute (xml_writer,
                                           BAD_CAST "RangFinal",
                                           "%d", rank->GetUIntValue ());
      }
      if (status)
      {
        gchar *xml_image = status->GetXmlImage ();

        xmlTextWriterWriteAttribute (xml_writer,
                                     BAD_CAST "Statut",
                                     BAD_CAST xml_image);
        g_free (xml_image);
      }
      xmlTextWriterEndElement (xml_writer);

      current = g_slist_next (current);
    }
  }
}

// --------------------------------------------------------------------------------
gboolean Stage::OnHttpPost (const gchar *command,
                            const gchar **ressource,
                            const gchar *data)
{
  return FALSE;
}

// --------------------------------------------------------------------------------
void Stage::Dump ()
{
  if (_result)
  {
    GSList *current = _result;

    for (guint i = 0; current != NULL; i++)
    {
      Player *player = (Player *) current->data;

      g_print ("%d >>> %s\n", i, player->GetName ());
      current = g_slist_next (current);
    }
  }
}

// --------------------------------------------------------------------------------
Object *Stage::GetPlayerDataOwner ()
{
  Module *module = dynamic_cast <Module *> (this);

  return module->GetDataOwner ();
}

// --------------------------------------------------------------------------------
void Stage::DrawConfig (GtkPrintOperation *operation,
                        GtkPrintContext   *context,
                        gint               page_nr)
{
}

// --------------------------------------------------------------------------------
void Stage::DrawConfigLine (GtkPrintOperation *operation,
                            GtkPrintContext   *context,
                            const gchar       *line)
{
  cairo_t *cr      = gtk_print_context_get_cairo_context (context);
  gdouble  paper_w = gtk_print_context_get_width (context);

  cairo_translate (cr,
                   0.0,
                   2.0 * paper_w / 100);

  {
    GooCanvas *canvas = Canvas::CreatePrinterCanvas (context);

    goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                         line,
                         1.0 * paper_w / 100,
                         0.0,
                         -1.0,
                         GTK_ANCHOR_W,
                         "font", BP_FONT "2px", NULL);

    goo_canvas_render (canvas,
                       gtk_print_context_get_cairo_context (context),
                       NULL,
                       1.0);

    gtk_widget_destroy (GTK_WIDGET (canvas));
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_all_radiobutton_toggled (GtkWidget *widget,
                                                            Object    *owner)
{
  Stage *stage = dynamic_cast <Stage *> (owner);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
  {
    stage->DeactivateNbQualified ();
  }
  else
  {
    stage->ActivateNbQualified ();
  }
}
