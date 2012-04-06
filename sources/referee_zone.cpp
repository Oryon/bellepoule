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

#include "player.hpp"
#include "canvas.hpp"
#include "stage.hpp"

#include "referee_zone.hpp"

// --------------------------------------------------------------------------------
RefereeZone::RefereeZone (Module *container)
  : DropZone (container)
{
  _referee_list = NULL;
}

// --------------------------------------------------------------------------------
RefereeZone::~RefereeZone ()
{
  while (_referee_list)
  {
    RemoveReferee ((Player *) _referee_list->data);
  }
}

// --------------------------------------------------------------------------------
void RefereeZone::BookReferees ()
{
  if (_container->GetState () == Module::OPERATIONAL)
  {
    GSList *current = _referee_list;

    while (current)
    {
      BookReferee ((Player *) current->data);
      current = g_slist_next (current);
    }
  }
}

// --------------------------------------------------------------------------------
void RefereeZone::FreeReferees ()
{
  if (_container->GetState () == Module::OPERATIONAL)
  {
    GSList *current = _referee_list;

    while (current)
    {
      FreeReferee ((Player *) current->data);
      current = g_slist_next (current);
    }
  }
}

// --------------------------------------------------------------------------------
void RefereeZone::BookReferee (Player *referee)
{
  Player::AttributeId attr_id ("availability");

  if (strcmp (referee->GetAttribute (&attr_id)->GetStrValue (),
              "Free") == 0)
  {
    referee->SetAttributeValue (&attr_id,
                                "Busy");
  }
}

// --------------------------------------------------------------------------------
void RefereeZone::FreeReferee (Player *referee)
{
  Player::AttributeId attr_id ("availability");
  Attribute           *attr = referee->GetAttribute (&attr_id);

  if (attr && strcmp (attr->GetStrValue (), "Busy") == 0)
  {
    referee->SetAttributeValue (&attr_id,
                                "Free");
  }
}

// --------------------------------------------------------------------------------
void RefereeZone::AddReferee (Player *referee)
{
  referee->AddMatchs (GetNbMatchs ());
  _container->RefreshMatchRate (referee);

  _referee_list = g_slist_prepend (_referee_list,
                                   referee);

  if (_container->GetState () == Module::OPERATIONAL)
  {
    BookReferee (referee);
  }
}

// --------------------------------------------------------------------------------
void RefereeZone::RemoveReferee (Player *referee)
{
  referee->RemoveMatchs (GetNbMatchs ());
  _container->RefreshMatchRate (referee);

  _referee_list = g_slist_remove (_referee_list,
                                  referee);

  {
    gboolean  locked = FALSE;
    Stage    *stage  = dynamic_cast <Stage *> (_container);

    if (stage)
    {
      locked = stage->Locked ();
    }

    if ((locked == FALSE) && (_container->GetState () != Module::LOADING))
    {
      FreeReferee (referee);
    }
  }
}

// --------------------------------------------------------------------------------
guint RefereeZone::GetNbMatchs ()
{
  return 0;
}

// --------------------------------------------------------------------------------
void RefereeZone::AddObject (Object *object)
{
  AddReferee ((Player *) object);
}

// --------------------------------------------------------------------------------
void RefereeZone::RemoveObject (Object *object)
{
  RemoveReferee ((Player *) object);
}
