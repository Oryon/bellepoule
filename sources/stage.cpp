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
#include <string.h>

#include "pool_allocator.hpp"
#include "pool_supervisor.hpp"
#include "classification.hpp"
#include "player.hpp"

#include "stage.hpp"

GSList *Stage::_stage_base = NULL;

// --------------------------------------------------------------------------------
Stage::Stage (StageClass *stage_class)
: Object ("Stage")
{
  _name                  = g_strdup ("");
  _locked                = false;
  _result                = NULL;
  _previous              = NULL;
  _next                  = NULL;
  _stage_class           = stage_class;
  _attendees             = NULL;
  _classification        = NULL;
  _input_provider        = NULL;
  _classification_on     = FALSE;

  _sensitivity_trigger    = new SensitivityTrigger ();
  _score_stuffing_trigger = NULL;

  _status_cbk_data = NULL;
  _status_cbk      = NULL;

  _nb_eliminated = new Data ("NbElimines",
                             (guint) 0);
}

// --------------------------------------------------------------------------------
Stage::~Stage ()
{
  FreeResult ();
  g_free (_name);

  _sensitivity_trigger->Release ();

  TryToRelease (_score_stuffing_trigger);
  TryToRelease (_classification);
  TryToRelease (_attendees);
  TryToRelease (_nb_eliminated);
}

// --------------------------------------------------------------------------------
void Stage::SetStatusCbk (StatusCbk  cbk,
                            void      *data)
{
  _status_cbk_data = data;
  _status_cbk      = cbk;
}

// --------------------------------------------------------------------------------
void Stage::SignalStatusUpdate ()
{
  if (_status_cbk)
  {
    _status_cbk (this,
                 _status_cbk_data);
  }
}

// --------------------------------------------------------------------------------
const gchar *Stage::GetClassName ()
{
  return _class->_name;
}

// --------------------------------------------------------------------------------
gchar *Stage::GetName ()
{
  return _name;
}

// --------------------------------------------------------------------------------
gchar *Stage::GetFullName ()
{
  return g_strdup_printf ("%s\n%s", _class->_name, _name);
}

// --------------------------------------------------------------------------------
void Stage::SetName (gchar *name)
{
  if (name)
  {
    g_free (_name);
    _name = NULL;

    _name = g_strdup (name);
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
}

// --------------------------------------------------------------------------------
void Stage::Lock (Reason reason)
{
  _locked = true;
  OnLocked (reason);
  SetResult ();
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

  _locked = false;
  OnUnLocked ();
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
void Stage::ApplyConfig ()
{
  Module *module = dynamic_cast <Module *> (this);

  if (module)
  {
    GtkWidget *w = module->GetWidget ("nb_eliminated_spinbutton");

    if (w)
    {
      _nb_eliminated->_value = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (w));
    }
  }
}

// --------------------------------------------------------------------------------
void Stage::FillInConfig ()
{
  Module *module = dynamic_cast <Module *> (this);

  if (module)
  {
    GtkWidget *w = module->GetWidget ("nb_eliminated_spinbutton");

    if (w)
    {
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (w),
                                 _nb_eliminated->_value);
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
void Stage::RetrieveAttendees ()
{
  if (_attendees)
  {
    _attendees->Release ();
  }

  if (_previous)
  {
    GSList *shortlist = _previous->GetOutputShortlist ();

    _attendees = new Attendees (_previous->_attendees,
                                shortlist);

    {
      Player::AttributeId  previous_rank_attr_id ("previous_stage_rank", this);
      GSList              *current = shortlist;

      for (guint i = 0; current != NULL; i++)
      {
        Player *player;

        player = (Player *) current->data;
        player->SetAttributeValue (&previous_rank_attr_id,
                                   i+1);
        {
          Player::AttributeId *status_attr_id;

          status_attr_id = new Player::AttributeId ("status", _previous);
          player->SetAttributeValue (status_attr_id,
                                     "Q");

          if (_previous->_classification)
          {
            status_attr_id = new Player::AttributeId ("status", _previous->_classification->GetDataOwner ());
            player->SetAttributeValue (status_attr_id,
                                       "Q");
          }
        }
        current = g_slist_next (current);
      }
    }
  }
  else
  {
    _attendees = new Attendees ();
  }
}

// --------------------------------------------------------------------------------
GSList *Stage::GetOutputShortlist ()
{
  GSList *shortlist = NULL;

  if (_result)
  {
    shortlist = g_slist_copy (_result);
  }

  {
    Classification *classification = GetClassification ();

    if (shortlist && classification)
    {
      guint nb_to_remove = _nb_eliminated->_value * g_slist_length (shortlist) / 100;

      for (guint i = 0; i < nb_to_remove; i++)
      {
        Player::AttributeId *attr_id;
        GSList              *current;
        Player              *player;

        current = g_slist_last (shortlist);
        player = (Player *) current->data;

        attr_id = new Player::AttributeId ("status", this);
        player->SetAttributeValue (attr_id,
                                   "N");

        attr_id = new Player::AttributeId ("status", classification->GetDataOwner ());
        player->SetAttributeValue (attr_id,
                                   "N");

        shortlist = g_slist_delete_link (shortlist,
                                         current);
      }
    }
  }

  return shortlist;
}

// --------------------------------------------------------------------------------
void Stage::LoadAttendees (xmlNode *n)
{
  if (_attendees == NULL)
  {
    if (_previous)
    {
      _attendees = new Attendees (_previous->_attendees,
                                  _previous->GetOutputShortlist ());
    }
    else
    {
      _attendees = new Attendees ();
    }
  }

  if (n)
  {
    gchar *ref_attr = (gchar *) xmlGetProp (n, BAD_CAST "REF");

    if (ref_attr)
    {
      Player *player = GetPlayerFromRef (atoi (ref_attr));

      if (player)
      {
        Player::AttributeId attr_id ("previous_stage_rank", this);
        gchar *rank_attr =  (gchar *) xmlGetProp (n, BAD_CAST "RangInitial");

        if (rank_attr)
        {
          player->SetAttributeValue (&attr_id,
                                     atoi (rank_attr));
        }
        else
        {
          player->SetAttributeValue (&attr_id,
                                     (guint) 0);
        }
      }
    }
  }
}

// --------------------------------------------------------------------------------
Player *Stage::GetPlayerFromRef (guint ref)
{
  GSList *shortlist = _attendees->GetShortList ();

  for (guint p = 0; p < g_slist_length (shortlist); p++)
  {
    Player *player;

    player = (Player *) g_slist_nth_data (shortlist,
                                          p);
    if (player->GetRef () == ref)
    {
      return player;
    }
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
    for (guint i = 0; i < g_slist_length (_stage_base); i++)
    {
      StageClass *stage_class;

      stage_class = (StageClass *) g_slist_nth_data (_stage_base,
                                                     i);
      if (strcmp (name, stage_class->_xml_name) == 0)
      {
        return stage_class;
      }
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
Stage::Rights Stage::GetRights ()
{
  return _stage_class->_rights;
}

// --------------------------------------------------------------------------------
const gchar *Stage::GetInputProviderClient ()
{
  return NULL;
}

// --------------------------------------------------------------------------------
void Stage::SetInputProvider (Stage *input_provider)
{
  _input_provider = input_provider;

  TryToRelease (_nb_eliminated);
  _nb_eliminated = input_provider->GetNbEliminated ();
  _nb_eliminated->Retain ();
}

// --------------------------------------------------------------------------------
Stage *Stage::GetInputProvider ()
{
  return _input_provider;
}

// --------------------------------------------------------------------------------
void Stage::ToggleClassification (gboolean classification_on)
{
  Module *module = dynamic_cast <Module *> (this);

  if (module)
  {
    GtkWidget *main_w           = module->GetWidget ("main_hook");
    GtkWidget *classification_w = module->GetWidget ("classification_hook");

    if (classification_on)
    {
      if (_result == NULL)
      {
        GSList *result = GetCurrentClassification ();

        UpdateClassification (result);
        g_slist_free (result);
      }
      else
      {
        UpdateClassification (_result);
      }

      if (main_w)
      {
        gtk_widget_hide_all (main_w);
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
Classification *Stage::GetClassification ()
{
  return _classification;
}

// --------------------------------------------------------------------------------
void Stage::SetClassificationFilter (Filter *filter)
{
  if (_classification == NULL)
  {
    Module *module = dynamic_cast <Module *> (this);
    GtkWidget *classification_w = module->GetWidget ("classification_list_host");

    if (classification_w == NULL)
    {
      classification_w = module->GetWidget ("classification_hook");
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
void Stage::UpdateClassification (GSList *result)
{
  if (_classification)
  {
    Player::AttributeId *previous_attr_id   = NULL;
    Player::AttributeId *rank_attr_id       = new Player::AttributeId ("rank", this);
    Player::AttributeId *final_rank_attr_id = new Player::AttributeId ("final_rank");

    if (_input_provider)
    {
      previous_attr_id = new Player::AttributeId ("rank", _previous);
    }

    _classification->Wipe ();

    {
      GSList *current_player = result;

      while (current_player)
      {
        Player    *player;
        Attribute *rank_attr;

        player = (Player *) current_player->data;

        rank_attr = player->GetAttribute (rank_attr_id);
        if (rank_attr)
        {
          if (previous_attr_id)
          {
            player->SetAttributeValue (previous_attr_id,
                                       (guint) rank_attr->GetValue ());
          }
          player->SetAttributeValue (final_rank_attr_id,
                                     (guint) rank_attr->GetValue ());
        }
        _classification->Add (player);

        current_player = g_slist_next (current_player);
      }
    }

    Object::TryToRelease (previous_attr_id);
    Object::TryToRelease (rank_attr_id);
    Object::TryToRelease (final_rank_attr_id);
  }
}

// --------------------------------------------------------------------------------
void Stage::SetResult ()
{
  GSList *result = GetCurrentClassification ();

  UpdateClassification (result);

  FreeResult ();
  _result = result;
}

// --------------------------------------------------------------------------------
void Stage::LockOnClassification (GtkWidget *w)
{
  _sensitivity_trigger->AddWidget (w);
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

      _score_stuffing_trigger->AddWidget (module->GetWidget ("stuff_toolbutton"));
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
void Stage::LoadConfiguration (xmlNode *xml_node)
{
  gchar *attr = (gchar *) xmlGetProp (xml_node,
                                      BAD_CAST "PhaseID");
  SetName (attr);

  if (_nb_eliminated)
  {
    _nb_eliminated->Load (xml_node);
  }
}

// --------------------------------------------------------------------------------
void Stage::SaveConfiguration (xmlTextWriter *xml_writer)
{
  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "PhaseID",
                                     "%s", GetName ());
  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "ID",
                                     "%d", _id);

  if (_nb_eliminated)
  {
    _nb_eliminated->Save (xml_writer);
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
      Attribute *previous_stage_rank;
      Attribute *status = NULL;

      player = (Player *) current->data;

      {
        Player::AttributeId *rank_attr_id;
        Player::AttributeId  previous_rank_attr_id ("previous_stage_rank", this);

        if (GetInputProviderClient ())
        {
          rank_attr_id = new Player::AttributeId ("rank", _next);
        }
        else
        {
          rank_attr_id = new Player::AttributeId ("rank", this);
        }

        rank                = player->GetAttribute (rank_attr_id);
        previous_stage_rank = player->GetAttribute (&previous_rank_attr_id);

        rank_attr_id->Release ();
      }

      if (_classification)
      {
        Player::AttributeId status_attr_id ("status", _classification->GetDataOwner ());

        status = player->GetAttribute (&status_attr_id);
      }
      else if (_next && _next->_input_provider && _next->_classification)
      {
        Player::AttributeId status_attr_id ("status", _next->_classification->GetDataOwner ());

        status = player->GetAttribute (&status_attr_id);
      }

      xmlTextWriterStartElement (xml_writer,
                                 BAD_CAST "Tireur");
      xmlTextWriterWriteFormatAttribute (xml_writer,
                                         BAD_CAST "REF",
                                         "%d", player->GetRef ());
      if (previous_stage_rank)
      {
        xmlTextWriterWriteFormatAttribute (xml_writer,
                                           BAD_CAST "RangInitial",
                                           "%d", (guint) previous_stage_rank->GetValue ());
      }
      if (rank)
      {
        xmlTextWriterWriteFormatAttribute (xml_writer,
                                           BAD_CAST "RangFinal",
                                           "%d", (guint)  rank->GetValue ());
      }
      if (status)
      {
        xmlTextWriterWriteFormatAttribute (xml_writer,
                                           BAD_CAST "Statut",
                                           "%s", (gchar *)  status->GetXmlImage ());
      }
      xmlTextWriterEndElement (xml_writer);

      current = g_slist_next (current);
    }
  }
}

// --------------------------------------------------------------------------------
Data *Stage::GetNbEliminated ()
{
  return _nb_eliminated;
}

// --------------------------------------------------------------------------------
void Stage::Dump ()
{
  if (_result)
  {
    for (guint i = 0; i < g_slist_length (_result); i++)
    {
      Player *player;

      player = (Player *) g_slist_nth_data (_result, i);

      g_print ("%d >>> %s\n", i, player->GetName ());
    }
  }
}
