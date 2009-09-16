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

#include "pool_allocator.hpp"
#include "pool_supervisor.hpp"
#include "player.hpp"

#include "stage.hpp"

GData *Stage_c::_stage_base = NULL;

// --------------------------------------------------------------------------------
Stage_c::Stage_c (gchar *class_name)
{
  _name       = g_strdup ("");
  _class_name = class_name;
  _locked     = false;
  _result     = NULL;
  _previous   = NULL;
}

// --------------------------------------------------------------------------------
Stage_c::~Stage_c ()
{
  FreeResult ();
  g_free (_name);
}

// --------------------------------------------------------------------------------
gchar *Stage_c::GetName ()
{
  return _name;
}

// --------------------------------------------------------------------------------
gchar *Stage_c::GetFullName ()
{
  return g_strdup_printf ("%s\n%s", _class_name, _name);
}

// --------------------------------------------------------------------------------
void Stage_c::SetName (gchar *name)
{
  if (name)
  {
    g_free (_name);
    _name = NULL;

    _name = g_strdup (name);
  }
}

// --------------------------------------------------------------------------------
void Stage_c::FreeResult ()
{
  g_slist_free (_result);
  _result = NULL;
}

// --------------------------------------------------------------------------------
void Stage_c::Lock ()
{
  _locked = true;
  OnLocked ();
}

// --------------------------------------------------------------------------------
void Stage_c::UnLock ()
{
  _locked = false;
  OnUnLocked ();
  FreeResult ();
}

// --------------------------------------------------------------------------------
gboolean Stage_c::Locked ()
{
  return _locked;
}

// --------------------------------------------------------------------------------
void Stage_c::SetPrevious (Stage_c *previous)
{
  _previous = previous;
}

// --------------------------------------------------------------------------------
Stage_c *Stage_c::GetPreviousStage ()
{
  return _previous;
}

// --------------------------------------------------------------------------------
GSList *Stage_c::GetResult ()
{
  return _result;
}

// --------------------------------------------------------------------------------
Player_c *Stage_c::GetPlayerFromRef (guint ref)
{
  for (guint p = 0; p < g_slist_length (_result); p++)
  {
    Player_c *player;

    player = (Player_c *) g_slist_nth_data (_result,
                                            p);
    if (player->GetRef () == ref)
    {
      return player;
    }
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void Stage_c::RegisterStageClass (const gchar *name,
                                  Creator      creator)
{
  if (_stage_base == NULL)
  {
    g_datalist_init (&_stage_base);
  }

  g_datalist_set_data (&_stage_base,
                       name,
                       (void *) creator);
}

// --------------------------------------------------------------------------------
Stage_c *Stage_c::CreateInstance (xmlNode *xml_node)
{
  if (xml_node)
  {
    Creator creator = (Creator) g_datalist_get_data (&_stage_base,
                                                     (gchar *) xml_node->name);

    if (creator)
    {
      return creator (xml_node);
    }
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void Stage_c::Dump ()
{
  if (_result)
  {
    for (guint i = 0; i < g_slist_length (_result); i++)
    {
      Player_c    *player;
      Attribute_c *attr;

      player = (Player_c *) g_slist_nth_data (_result, i);
      attr = player->GetAttribute ("name");

      g_print ("%d >>> %s\n", i, attr->GetStringImage ());
    }
  }
}
