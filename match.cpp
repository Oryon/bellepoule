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
#include <goocanvas.h>

#include "player.hpp"
#include "match.hpp"

// --------------------------------------------------------------------------------
Match_c::Match_c  (Player_c *A,
                   Player_c *B)
{
  A->Retain ();
  B->Retain ();

  _A = A;
  _B = B;

  _A_score = new Score_c;
  _B_score = new Score_c;
}

// --------------------------------------------------------------------------------
Match_c::~Match_c ()
{
  Object_c::Release (_A);
  Object_c::Release (_B);
  Object_c::Release (_A_score);
  Object_c::Release (_B_score);
}

// --------------------------------------------------------------------------------
Player_c *Match_c::GetPlayerA ()
{
  return _A;
}

// --------------------------------------------------------------------------------
Player_c *Match_c::GetPlayerB ()
{
  return _B;
}

// --------------------------------------------------------------------------------
gboolean Match_c::HasPlayer (Player_c *player)
{
  return ((_A == player) || (_B == player));
}

// --------------------------------------------------------------------------------
gboolean Match_c::PlayerHasScore (Player_c *player)
{
  if (player == _A)
  {
    return (_A_score->IsKnown ());
  }
  else if (player == _B)
  {
    return (_B_score->IsKnown ());
  }
  else
  {
    return false;
  }
}

// --------------------------------------------------------------------------------
void Match_c::SetScore (Player_c *player,
                        guint     score)
{
  if (_A == player)
  {
    _A_score->Set (score);
  }
  else if (_B == player)
  {
    _B_score->Set (score);
  }
}

// --------------------------------------------------------------------------------
Score_c *Match_c::GetScore (Player_c *player)
{
  if (_A == player)
  {
    return _A_score;
  }
  else if (_B == player)
  {
    return _B_score;
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void Match_c::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST "match");

  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "player_A",
                                     "%d", _A->GetRef ());
  if (_A_score->IsKnown ())
  {
    xmlTextWriterWriteFormatAttribute (xml_writer,
                                       BAD_CAST "score_A",
                                       "%d", _A_score->Get ());
  }

  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "player_B",
                                     "%d", _B->GetRef ());
  if (_B_score->IsKnown ())
  {
    xmlTextWriterWriteFormatAttribute (xml_writer,
                                       BAD_CAST "score_B",
                                       "%d", _B_score->Get ());
  }

  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void Match_c::CleanScore ()
{
  _A_score->Clean ();
  _B_score->Clean ();
}
