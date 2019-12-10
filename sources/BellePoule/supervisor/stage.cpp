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
#include <libxml/xpath.h>

#include "util/player.hpp"
#include "util/canvas.hpp"
#include "util/attribute.hpp"
#include "util/attribute_desc.hpp"
#include "util/filter.hpp"
#include "util/data.hpp"
#include "util/xml_scheme.hpp"
#include "util/fie_time.hpp"
#include "network/advertiser.hpp"
#include "network/message.hpp"
#include "attendees.hpp"
#include "classification.hpp"
#include "contest.hpp"
#include "book/section.hpp"
#include "match.hpp"

#include "stage.hpp"

GSList *Stage::_stage_base = nullptr;

// --------------------------------------------------------------------------------
Stage::Stage (StageClass *stage_class)
{
  _name              = g_strdup ("");
  _locked            = FALSE;
  _result            = nullptr;
  _output_short_list = nullptr;
  _quota_exceedance  = 0;
  _previous          = nullptr;
  _next              = nullptr;
  _stage_class       = stage_class;
  _attendees         = nullptr;
  _classification    = nullptr;
  _input_provider    = nullptr;
  _classification_on = FALSE;

  _sensitivity_trigger    = new SensitivityTrigger ();
  _score_stuffing_trigger = nullptr;

  _status_listener = nullptr;

  _max_score = nullptr;

  _nb_qualified = new Data ("NbQualifiesParIndice",
                            (guint) 0);
  DeactivateNbQualified ();
}

// --------------------------------------------------------------------------------
Stage::~Stage ()
{
  FreeResult ();
  g_free (_name);

  TryToRelease (_sensitivity_trigger);
  TryToRelease (_score_stuffing_trigger);
  TryToRelease (_classification);
  TryToRelease (_attendees);
  TryToRelease (_nb_qualified);

  if (_previous)
  {
    _previous->_next = nullptr;
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
const gchar *Stage::GetPurpose ()
{
  return _class->_name;
}

// --------------------------------------------------------------------------------
GList *Stage::GetBookSections (StageView view)
{
  GList       *sections = nullptr;
  gchar       *title    = GetFullName ();
  BookSection *section  = new BookSection (title);

  g_free (title);

  return g_list_append (sections,
                        section);
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
guint Stage::GetNetID ()
{
  if (_parcel)
  {
    return _parcel->GetNetID ();
  }

  return 0;
}

// --------------------------------------------------------------------------------
void Stage::FreeResult ()
{
  g_slist_free (_result);
  _result = nullptr;

  g_slist_free (_output_short_list);
  _output_short_list = nullptr;
  _quota_exceedance  = 0;
}

// --------------------------------------------------------------------------------
void Stage::Reset ()
{
  Player::AttributeId status_attr_id        ("status", GetPlayerDataOwner ());
  Player::AttributeId global_status_attr_id ("global_status");
  Player::AttributeId final_rank_attr_id    ("final_rank");

  for (GSList *current = GetShortList (); current; current = g_slist_next (current))
  {
    Player *player = (Player *) current->data;

    player->SetAttributeValue (&status_attr_id,
                               "Q");
    player->SetAttributeValue (&global_status_attr_id,
                               "Q");

    player->RemoveAttribute (&final_rank_attr_id);
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

  if (GetInputProvider ())
  {
    _previous->Spread ();
  }
}

// --------------------------------------------------------------------------------
void Stage::UnLock ()
{
  FreeResult ();

  _locked = FALSE;
  OnUnLocked ();

  if (GetInputProvider ())
  {
    _previous->Spread ();
  }
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
Error *Stage::GetError ()
{
  return nullptr;
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
      else
      {
        GtkWidget *combobox = GTK_WIDGET (module->GetGObject ("name_combobox"));

        if (combobox)
        {
          GtkEntry *child = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (combobox)));
          gchar    *name  = (gchar *) gtk_entry_get_text (child);

          SetName (name);
        }
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
    InitQualifiedForm ();

    {
      GtkEntry *w = GTK_ENTRY (module->GetGObject ("name_entry"));

      if (w)
      {
        gtk_entry_set_text (w,
                            GetName ());
      }
      else
      {
        GtkWidget *combobox = GTK_WIDGET (module->GetGObject ("name_combobox"));

        if (combobox)
        {
          GtkEntry *child = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (combobox)));

          if (child)
          {
            gtk_entry_set_text (child,
                                GetName ());
          }
        }
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
  GSList *short_list = GetShortList ();

  if (short_list)
  {
    // Status
    {
      Player::AttributeId  stage_start_rank_attr_id ("stage_start_rank", GetPlayerDataOwner ());
      Player::AttributeId  status_attr_id ("status", GetPlayerDataOwner ());
      Player::AttributeId  global_status_attr_id ("global_status");
      GSList              *current = short_list;

      for (guint i = 0; current != nullptr; i++)
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
      _nb_qualified->_value = g_slist_length (short_list);
    }
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
    _output_short_list = nullptr;
  }

  if (_result)
  {
    _output_short_list = g_slist_copy (_result);
  }

  if (_output_short_list && _classification)
  {
    Player::AttributeId classif_attr_id       ("status", GetPlayerDataOwner ());
    Player::AttributeId global_status_attr_id ("global_status");

    // Remove all of the withdrawalls and black cards
    {
      Player::AttributeId stage_status_attr_id ("status", GetPlayerDataOwner ());
      GSList *new_short_list = nullptr;
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
          Attribute           *last_qualified_rank = nullptr;

          _quota_exceedance = g_slist_length (_output_short_list);
          for (guint i = 0; current != nullptr; i++)
          {
            Player    *current_fencer = (Player *) current->data;
            Attribute *current_rank   = current_fencer->GetAttribute (&rank_attr_id);

            if (last_qualified_rank == nullptr)
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
  if (_nb_qualified->IsValid () == FALSE)
  {
    _nb_qualified->_value = g_slist_length (GetShortList ());
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
  for (GSList *current = GetShortList (); current; current = g_slist_next (current))
  {
    Player *player = (Player *) current->data;

    if (player->GetRef () == ref)
    {
      return player;
    }
  }

  return nullptr;
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
  return nullptr;
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
void Stage::ShareAttendees (Stage *with)
{
  if (with)
  {
    _attendees = with->_attendees;
    _attendees->Retain ();
  }
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

  return nullptr;
}

// --------------------------------------------------------------------------------
guint Stage::GetRights ()
{
  return _stage_class->_rights;
}

// --------------------------------------------------------------------------------
const gchar *Stage::GetInputProviderClient ()
{
  return nullptr;
}

// --------------------------------------------------------------------------------
gchar *Stage::GetAnnounce ()
{
  gchar       *name  = GetName ();
  const gchar *emoji = "\xf0\x9f\x8f\x81";
  //const  gchar *emoji = "\xe2\x98\x91";

  if (name[0])
  {
    return g_strdup_printf ("%s '%s' %s", _class->_name, GetName (), emoji);
  }
  else
  {
    return g_strdup_printf ("%s %s", _class->_name, emoji);
  }
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
  GtkToggleToolButton *w      = nullptr;

  if (classification_toggle_button)
  {
    w = GTK_TOGGLE_TOOL_BUTTON (module->GetGObject (classification_toggle_button));
  }

  if ((w == nullptr) || gtk_toggle_tool_button_get_active (w))
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
        _sensitivity_trigger->SwitchOff ();
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
        _sensitivity_trigger->SwitchOn ();
      }
    }
  }

  _classification_on = classification_on;
}

// --------------------------------------------------------------------------------
GSList *Stage::GetShortList ()
{
  if (_previous && (_previous->_locked == TRUE))
  {
    return _previous->_output_short_list;
  }

  return nullptr;
}

// --------------------------------------------------------------------------------
Classification *Stage::GetClassification ()
{
  return _classification;
}

// --------------------------------------------------------------------------------
void Stage::SetClassificationFilter (Filter *filter)
{
  if (_classification == nullptr)
  {
    Module    *module           = dynamic_cast <Module *> (this);
    GtkWidget *classification_w = GTK_WIDGET (module->GetGObject ("coumpound_classification_hook"));

    if (classification_w == nullptr)
    {
      classification_w = GTK_WIDGET (module->GetGObject ("classification_hook"));
    }

    if (classification_w)
    {
      _classification = new Classification ();
      _classification->SetFilter (filter);
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
  if (_result && (GetInputProviderClient () == nullptr))
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

      if ((_next == nullptr) || (_next->HasItsOwnRanking () == FALSE))
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
    Player::AttributeId *previous_attr_id = nullptr;

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
  // Reset status from N to Q before
  {
    Player::AttributeId classif_attr_id       ("status", GetPlayerDataOwner ());
    Player::AttributeId global_status_attr_id ("global_status");

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

  {
    GSList *result = GetCurrentClassification ();

    FreeResult ();
    _result = result;

    SetOutputShortlist ();

    UpdateClassification (_classification,
                          result);
  }
}

// --------------------------------------------------------------------------------
void Stage::LockOnClassification (GtkWidget *w)
{
  _sensitivity_trigger->AddWidget (w);
}

// --------------------------------------------------------------------------------
void Stage::SetScoreStuffingPolicy (gboolean allowed)
{
  if (_score_stuffing_trigger == nullptr)
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

  if (_parcel)
  {
    gchar *attr = (gchar *) xmlGetProp (xml_node, BAD_CAST "NetID");

    if (attr)
    {
      _parcel->SetNetID (g_ascii_strtoull (attr,
                                           nullptr,
                                           16));
      xmlFree (attr);
    }
  }

  {
    Module *module = dynamic_cast <Module *> (this);

    if (module)
    {
      Filter *filter = module->GetFilter ();

      if (_input_provider)
      {
        filter->Load (xml_node,
                      "Scores");
      }
      else
      {
        filter->Load (xml_node);
      }
    }
  }

  if (_classification)
  {
    Module *module = dynamic_cast <Module *> (_classification);

    if (module)
    {
      Filter *filter = module->GetFilter ();

      filter->Load (xml_node,
                    "Classement");
    }
  }
}

// --------------------------------------------------------------------------------
void Stage::LoadMatch (xmlNode *xml_node,
                       Match   *match)
{
  for (xmlNode *n = xml_node; n != nullptr; n = n->next)
  {
    if (n->type == XML_ELEMENT_NODE)
    {
      static xmlNode *A = nullptr;
      static xmlNode *B = nullptr;

      if (g_strcmp0 ((char *) n->name, "Match") == 0)
      {
        gchar *attr;

        A = nullptr;
        B = nullptr;

        attr = (gchar *) xmlGetProp (n, BAD_CAST "ID");
        if ((attr == nullptr) || (atoi (attr) != match->GetNumber ()))
        {
          xmlFree (attr);
          return;
        }
        xmlFree (attr);

        attr = (gchar *) xmlGetProp (xml_node, BAD_CAST "Piste");
        if (attr)
        {
          match->SetPiste (atoi (attr));
          xmlFree (attr);
        }

        {
          gchar *date = (gchar *) xmlGetProp (xml_node, BAD_CAST "Date");
          gchar *time = (gchar *) xmlGetProp (xml_node, BAD_CAST "Heure");

          if (date && time)
          {
            match->SetStartTime (new FieTime (date, time));
          }

          xmlFree (date);
          xmlFree (time);
        }

        attr = (gchar *) xmlGetProp (xml_node, BAD_CAST "Duree");
        if (attr)
        {
          match->SetDuration (atoi (attr));
          xmlFree (attr);
        }

        attr = (gchar *) xmlGetProp (xml_node, BAD_CAST "Portee");
        if (attr)
        {
          match->SetDurationSpan (atoi (attr));
          xmlFree (attr);
        }
      }
      else if (g_strcmp0 ((char *) n->name, "Arbitre") == 0)
      {
        gchar *attr = (gchar *) xmlGetProp (n, BAD_CAST "REF");

        if (attr)
        {
          Player *referee = _contest->GetRefereeFromRef (atoi (attr));

          match->AddReferee (referee);
          xmlFree (attr);
        }
      }
      else if (g_strcmp0 ((char *) n->name, GetXmlPlayerTag ()) == 0)
      {
        if (A == nullptr)
        {
          A = n;
        }
        else
        {
          B = n;

          {
            Player *fencer_a = nullptr;
            Player *fencer_b = nullptr;
            gchar  *attr;

            attr = (gchar *) xmlGetProp (A, BAD_CAST "REF");
            if (attr)
            {
              fencer_a = GetFencerFromRef (atoi (attr));
              xmlFree (attr);
            }

            attr = (gchar *) xmlGetProp (B, BAD_CAST "REF");
            if (attr)
            {
              fencer_b = GetFencerFromRef (atoi (attr));
              xmlFree (attr);
            }

            if (fencer_a && fencer_b)
            {
              match->Load (A, fencer_a,
                           B, fencer_b);
            }
          }

          A = nullptr;
          B = nullptr;
          return;
        }
      }
      LoadMatch (n->children,
                 match);
    }
  }
}

// --------------------------------------------------------------------------------
void Stage::SaveConfiguration (XmlScheme *xml_scheme)
{
  if (GetInputProviderClient ())
  {
    Stage *next_stage = GetNextStage ();

    if (next_stage)
    {
      xml_scheme->WriteFormatAttribute ("PhaseID",
                                        "%d", next_stage->GetId ());
    }
  }
  else
  {
    xml_scheme->WriteFormatAttribute ("PhaseID",
                                      "%d", _id);
  }

  xml_scheme->WriteAttribute ("ID",
                              GetName ());

  if (_parcel)
  {
    xml_scheme->WriteFormatAttribute ("NetID",
                                      "%x", _parcel->GetNetID ());
  }

  if (_max_score)
  {
    _max_score->Save (xml_scheme);
  }

  if (_nb_qualified->IsValid ())
  {
    _nb_qualified->Save (xml_scheme);
  }

  SaveFilters (xml_scheme);
}

// --------------------------------------------------------------------------------
void Stage::SaveFilters (XmlScheme   *xml_scheme,
                         const gchar *as)
{
  {
    Module *module = dynamic_cast <Module *> (this);

    if (module)
    {
      Filter *filter = module->GetFilter ();

      filter->Save (xml_scheme,
                    as);
    }
  }

  if (_classification)
  {
    Module *module = dynamic_cast <Module *> (_classification);

    if (module)
    {
      Filter *filter = module->GetFilter ();

      filter->Save (xml_scheme,
                    "Classement");
    }
  }

  if (_next && GetInputProviderClient ())
  {
    _next->SaveFilters (xml_scheme,
                        "Scores");
  }
}

// --------------------------------------------------------------------------------
void Stage::Load (xmlNode *n)
{
  LoadAttendees (n);
}

// --------------------------------------------------------------------------------
void Stage::Save (XmlScheme *xml_scheme)
{
  xml_scheme->StartElement (_class->_xml_name);

  SaveConfiguration (xml_scheme);
  SaveAttendees     (xml_scheme);

  xml_scheme->EndElement ();
}

// --------------------------------------------------------------------------------
void Stage::SaveAttendees (XmlScheme *xml_scheme)
{
  for (GSList *current = GetShortList (); current; current = g_slist_next (current))
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

    xml_scheme->StartElement (player->GetXmlTag ());
    xml_scheme->WriteFormatAttribute ("REF",
                                      "%d", player->GetRef ());
    if (stage_start_rank)
    {
      xml_scheme->WriteFormatAttribute ("RangInitial",
                                        "%d", stage_start_rank->GetUIntValue ());
    }
    if (rank)
    {
      xml_scheme->WriteFormatAttribute ("RangFinal",
                                        "%d", rank->GetUIntValue ());
    }
    if (status)
    {
      gchar *xml_image = status->GetXmlImage ();

      xml_scheme->WriteAttribute ("Statut",
                                  xml_image);
      g_free (xml_image);
    }
    xml_scheme->EndElement ();
  }
}

// --------------------------------------------------------------------------------
void Stage::Dump ()
{
  if (_result)
  {
    GSList *current = _result;

    for (guint i = 0; current != nullptr; i++)
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
                       nullptr,
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
