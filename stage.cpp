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
  _stage_class           = stage_class;
  _attendees             = NULL;
  _classification        = NULL;
  _classification_filter = NULL;
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

  TryToRelease (_classification_filter);
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
void Stage::FreeResult ()
{
  g_slist_free (_result);
  _result = NULL;
}

// --------------------------------------------------------------------------------
void Stage::Lock ()
{
  _locked = true;
  OnLocked ();
  SetResult ();
}

// --------------------------------------------------------------------------------
void Stage::UnLock ()
{
  FreeResult ();

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
    g_slist_free (_attendees);
    _attendees = NULL;
  }

  if (_previous)
  {
    _attendees = g_slist_copy (_previous->_result);

    {
      Player::AttributeId  attr_id ("previous_stage_rank", this);
      GSList              *current = _attendees;

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
}

// --------------------------------------------------------------------------------
Player *Stage::GetPlayerFromRef (guint ref)
{
  for (guint p = 0; p < g_slist_length (_attendees); p++)
  {
    Player *player;

    player = (Player *) g_slist_nth_data (_attendees,
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
void Stage::GetStageClass (guint    index,
                           gchar   **xml_name,
                           Creator *creator,
                           Rights  *rights)
{
  StageClass *stage_class = (StageClass *) g_slist_nth_data (_stage_base,
                                                             index);
  if (stage_class)
  {
    *xml_name = (gchar *) stage_class->_xml_name;
    *creator  = stage_class->_creator;
    *rights   = stage_class->_rights;
  }
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
        gtk_widget_hide (main_w);
      }
      gtk_widget_show (classification_w);

      _sensitivity_trigger->SwitchOff ();
    }
    else
    {
      gtk_widget_hide (classification_w);
      if (main_w)
      {
        gtk_widget_show (main_w);
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
  _classification_filter = filter;
  _classification_filter->Retain ();
}

// --------------------------------------------------------------------------------
void Stage::UpdateClassification (GSList *result)
{
  Module    *module           = dynamic_cast <Module *> (this);
  GtkWidget *classification_w = module->GetWidget ("classification_hook");

  if (classification_w)
  {
    Player::AttributeId attr_id ("rank", this);

    if (_classification == NULL)
    {
      _classification = new Classification (_classification_filter);

      _classification->SetDataOwner (this);
      module->Plug (_classification,
                    classification_w);
    }

    _classification->Wipe ();

    for (guint i = 0; i < g_slist_length (result); i ++)
    {
      Player *player;

      player = (Player *) g_slist_nth_data (result,
                                            i);

      player->SetAttributeValue (&attr_id,
                                 i + 1);
      _classification->Add (player);
    }
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
                                      BAD_CAST "name");
  SetName (attr);
}

// --------------------------------------------------------------------------------
void Stage::SaveConfiguration (xmlTextWriter *xml_writer)
{
  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "name",
                                     "%s", GetName ());
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
