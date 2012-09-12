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
  _referee_list   = NULL;
  _already_booked = TRUE;
}

// --------------------------------------------------------------------------------
RefereeZone::~RefereeZone ()
{
}

// --------------------------------------------------------------------------------
void RefereeZone::BookReferees ()
{
  if (_already_booked == FALSE)
  {
    GSList *current = _referee_list;

    while (current)
    {
      BookReferee ((Player *) current->data);

      current = g_slist_next (current);
    }

    _already_booked = TRUE;
  }
}

// --------------------------------------------------------------------------------
void RefereeZone::FreeReferees ()
{
  if (_already_booked == TRUE)
  {
    GSList *current = _referee_list;

    while (current)
    {
      FreeReferee ((Player *) current->data);

      current = g_slist_next (current);
    }

    _already_booked = FALSE;
  }
}

// --------------------------------------------------------------------------------
void RefereeZone::BookReferee (Player *referee)
{
  guint token = referee->GetUIntData (NULL,
                                      "Referee::token");

  token++;

  if (token == 1)
  {
    Player::AttributeId attr_id ("availability");

    if (strcmp (referee->GetAttribute (&attr_id)->GetStrValue (),
                "Free") == 0)
    {
      referee->SetAttributeValue (&attr_id,
                                  "Busy");
    }
  }

  referee->SetData (NULL,
                    "Referee::token",
                    (void *) token);
}

// --------------------------------------------------------------------------------
void RefereeZone::FreeReferee (Player *referee)
{
  guint token = referee->GetUIntData (NULL,
                                      "Referee::token");

  if (token > 0)
  {
    token--;
  }

  if (token == 0)
  {
    Player::AttributeId attr_id ("availability");
    Attribute           *attr = referee->GetAttribute (&attr_id);

    if (attr && strcmp (attr->GetStrValue (), "Busy") == 0)
    {
      referee->SetAttributeValue (&attr_id,
                                  "Free");
    }
  }

  referee->SetData (NULL,
                    "Referee::token",
                    (void *) token);
}

// --------------------------------------------------------------------------------
void RefereeZone::AddReferee (Player *referee)
{
  if (g_slist_find (_referee_list,
                    referee) == NULL)
  {
    referee->AddMatchs (GetNbMatchs ());
    _container->RefreshMatchRate (referee);

    _referee_list = g_slist_prepend (_referee_list,
                                     referee);

    if (_already_booked == TRUE)
    {
      BookReferee (referee);
    }
  }
}

// --------------------------------------------------------------------------------
void RefereeZone::RemoveReferee (Player *referee)
{
  referee->RemoveMatchs (GetNbMatchs ());
  _container->RefreshMatchRate (referee);

  _referee_list = g_slist_remove (_referee_list,
                                  referee);

  if (_already_booked == TRUE)
  {
    FreeReferee (referee);
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
