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

  _sensitivity_trigger    = new SensitivityTrigger ();
  _score_stuffing_trigger = NULL;

  _status_cbk_data = NULL;
  _status_cbk      = NULL;
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
}

// --------------------------------------------------------------------------------
void Stage::FillInConfig ()
{
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
      Player::AttributeId  attr_id ("previous_stage_rank", this);
      GSList              *current = shortlist;

      for (guint i = 0; current != NULL; i++)
      {
        Player *player;

        player = (Player *) current->data;
        player->SetAttributeValue (&attr_id,
                                   i+1);
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
  if (_result)
  {
    return g_slist_copy (_result);
  }
  else
  {
    return NULL;
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
                                  _previous->GetOutputShortlist ());
    }
    else
    {
      _attendees = new Attendees ();
    }
  }

  if (n)
  {
    gchar *attr = (gchar *) xmlGetProp (n, BAD_CAST "REF");

    if (attr)
    {
      Player *player = GetPlayerFromRef (atoi (attr));

      if (player)
      {
        Player::AttributeId attr_id ("previous_stage_rank", this);
        gchar *attr =  (gchar *) xmlGetProp (n, BAD_CAST "RangInitial");

        if (attr)
        {
          player->SetAttributeValue (&attr_id,
                                     atoi (attr));
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

      if (main_w)
      {
        gtk_widget_hide_all (main_w);
      }
      gtk_widget_show (classification_w);

      _sensitivity_trigger->SwitchOff ();
    }
    else
    {
      gtk_widget_hide (classification_w);
      if (main_w)
      {
        gtk_widget_show_all (main_w);
      }

      _sensitivity_trigger->SwitchOn ();
    }
  }
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
    Player::AttributeId *attr_id            = new Player::AttributeId ("rank", this);
    Player::AttributeId *final_rank_attr_id = new Player::AttributeId ("final_rank");

    if (_input_provider)
    {
      previous_attr_id = new Player::AttributeId ("rank", _previous);
    }

    _classification->Wipe ();

    for (guint i = 0; i < g_slist_length (result); i ++)
    {
      Player *player;

      player = (Player *) g_slist_nth_data (result,
                                            i);

      if (previous_attr_id)
      {
        player->SetAttributeValue (previous_attr_id,
                                   i + 1);
      }
      player->SetAttributeValue (attr_id,
                                 i + 1);
      player->SetAttributeValue (final_rank_attr_id,
                                 i + 1);
      _classification->Add (player);
    }

    Object::TryToRelease (previous_attr_id);
    Object::TryToRelease (attr_id);
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
                                           "%d",(guint)  rank->GetValue ());
      }
      xmlTextWriterWriteAttribute (xml_writer,
                                   BAD_CAST "Statut",
                                   BAD_CAST "Q");
      xmlTextWriterEndElement (xml_writer);

      current = g_slist_next (current);
    }
  }
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
