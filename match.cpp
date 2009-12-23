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
#include <goocanvas.h>

#include "player.hpp"
#include "match.hpp"

// --------------------------------------------------------------------------------
Match_c::Match_c (guint max_score)
{
  _max_score = max_score;

  _A = NULL;
  _B = NULL;

  _A_is_known = FALSE;
  _B_is_known = FALSE;

  _A_score = new Score_c (max_score);
  _B_score = new Score_c (max_score);
}

// --------------------------------------------------------------------------------
Match_c::Match_c  (Player_c *A,
                   Player_c *B,
                   guint     max_score)
{
  _max_score = max_score;

  _A = A;
  _B = B;

  _A_is_known = TRUE;
  _B_is_known = TRUE;

  _A_score = new Score_c (max_score);
  _B_score = new Score_c (max_score);
}

// --------------------------------------------------------------------------------
Match_c::~Match_c ()
{
  Object_c::Release (_A_score);
  Object_c::Release (_B_score);
}

// --------------------------------------------------------------------------------
void Match_c::SetPlayerA (Player_c *player)
{
  _A          = player;
  _A_is_known = TRUE;
}

// --------------------------------------------------------------------------------
void Match_c::SetPlayerB (Player_c *player)
{
  _B          = player;
  _B_is_known = TRUE;
}

// --------------------------------------------------------------------------------
void Match_c::SetMaxScore (guint max_score)
{
  _max_score = max_score;

  _A_score->SetMaxScore (max_score);
  _B_score->SetMaxScore (max_score);
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
Player_c *Match_c::GetWinner ()
{
  if (   (_B == NULL)
      && _B_is_known)
  {
    return _A;
  }
  else if (   (_A == NULL)
           && _A_is_known)
  {
    return _B;
  }
  else if (PlayerHasScore (_A) && PlayerHasScore (_B))
  {
    Score_c *score_A = GetScore (_A);
    Score_c *score_B = GetScore (_B);

    if (   score_A->IsValid ()
        && score_B->IsValid ()
        && score_A->IsConsistentWith (score_B))
    {
      if (_A_score->Get () >= _B_score->Get ())
      {
        return _A;
      }
      else
      {
        return _B;
      }
    }
  }

  return NULL;
}

// --------------------------------------------------------------------------------
gboolean Match_c::HasSinglePlayer ()
{
  return (   ((_A != NULL) && (_B == NULL))
          || ((_B != NULL) && (_A == NULL)));
}

// --------------------------------------------------------------------------------
gboolean Match_c::HasPlayer (Player_c *player)
{
  return ((_A == player) || (_B == player));
}

// --------------------------------------------------------------------------------
gboolean Match_c::PlayerHasScore (Player_c *player)
{
  if (_A && (player == _A))
  {
    return (_A_score->IsKnown ());
  }
  else if (_B && (player == _B))
  {
    return (_B_score->IsKnown ());
  }
  else
  {
    return FALSE;
  }
}

// --------------------------------------------------------------------------------
void Match_c::SetScore (Player_c *player,
                        gint      score)
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
gboolean Match_c::ScoreIsNumber (gchar *score)
{
  for (guint i = 0; i < strlen (score); i++)
  {
    if (g_ascii_isalpha (score[i]))
    {
      return false;
    }
  }

  return true;
}

// --------------------------------------------------------------------------------
gboolean Match_c::SetScore (Player_c *player,
                            gchar    *score)
{
  gboolean result = false;

  if (score)
  {
    if (   (strlen (score) == 1)
        && (g_ascii_toupper (score[0]) == 'V'))
    {
      SetScore (player,
                _max_score);
      result = true;
    }
    else if (ScoreIsNumber (score))
    {
      gchar *max_str        = g_strdup_printf ("%d", _max_score);
      gchar *one_digit_more = g_strdup_printf ("%s0", score);

      if (strlen (score) >= strlen (max_str))
      {
        SetScore (player,
                  atoi (score));
        result = true;
      }
      else if ((guint) atoi (one_digit_more) > _max_score)
      {
        SetScore (player,
                  atoi (score));
        result = true;
      }

      g_free (one_digit_more);
      g_free (max_str);
    }
  }

  if (result == FALSE)
  {
    SetScore (player,
              -1);
  }

  return result;
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

  if (_A)
  {
    xmlTextWriterWriteFormatAttribute (xml_writer,
                                       BAD_CAST "player_A",
                                       "%d", _A->GetRef ());
    if (_A_score->IsKnown ())
    {
      xmlTextWriterWriteFormatAttribute (xml_writer,
                                         BAD_CAST "score_A",
                                         "%d", _A_score->Get ());
    }
  }

  if (_B)
  {
    xmlTextWriterWriteFormatAttribute (xml_writer,
                                       BAD_CAST "player_B",
                                       "%d", _B->GetRef ());
    if (_B_score->IsKnown ())
    {
      xmlTextWriterWriteFormatAttribute (xml_writer,
                                         BAD_CAST "score_B",
                                         "%d", _B_score->Get ());
    }
  }

  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void Match_c::CleanScore ()
{
  _A_score->Clean ();
  _B_score->Clean ();
}
