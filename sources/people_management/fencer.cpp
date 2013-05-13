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

#include "player_factory.hpp"
#include "fencer.hpp"

const gchar *Fencer::_class_name = "Fencer";
const gchar *Fencer::_xml_tag    = "Tireur";

// --------------------------------------------------------------------------------
Fencer::Fencer ()
: Player (_class_name)
{
  _team = NULL;
}

// --------------------------------------------------------------------------------
Fencer::~Fencer ()
{
  Object::TryToRelease (_team);
}

// --------------------------------------------------------------------------------
void Fencer::SetTeam (Team *team)
{
  if (_team)
  {
    _team->RemoveMember (this);
    _team->Release ();
  }

  _team = team;
  if (_team)
  {
    _team->Retain ();
    _team->AddMember (this);
  }
}

// --------------------------------------------------------------------------------
Team *Fencer::GetTeam ()
{
  return _team;
}

// --------------------------------------------------------------------------------
Player *Fencer::Clone ()
{
  return new Fencer ();
}

// --------------------------------------------------------------------------------
void Fencer::RegisterPlayerClass ()
{
  PlayerFactory::AddPlayerClass (_class_name,
                                 _xml_tag,
                                 CreateInstance);
}

// --------------------------------------------------------------------------------
const gchar *Fencer::GetXmlTag ()
{
  return _xml_tag;
}

// --------------------------------------------------------------------------------
Player *Fencer::CreateInstance ()
{
  return new Fencer ();
}
