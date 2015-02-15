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

#include "score.hpp"

// --------------------------------------------------------------------------------
Score::Score  (Data *max)
  : Object ("Score")
{
  _max           = max;
  _score         = 0;
  _status        = UNKNOWN;
  _backup_status = UNKNOWN;
}

// --------------------------------------------------------------------------------
Score::~Score ()
{
}

// --------------------------------------------------------------------------------
gboolean Score::IsKnown ()
{
  return _status != UNKNOWN;
}

// --------------------------------------------------------------------------------
gboolean Score::IsTheBest ()
{
  return _status == VICTORY;
}

// --------------------------------------------------------------------------------
guint Score::Get ()
{
  if ((_status == VICTORY) || (_status == DEFEAT))
  {
    return _score;
  }

  return 0;
}

// --------------------------------------------------------------------------------
gchar *Score::GetImage ()
{
  gchar *image;

  if (_status == UNKNOWN)
  {
    image = g_new (char, 1);
    image[0] = 0; // empty string
  }
  else if (_status == VICTORY)
  {
    if (_score == _max->_value)
    {
      image = g_strdup_printf ("V");
    }
    else
    {
      image = g_strdup_printf ("V%d", _score);
    }
  }
  else if (_status == DEFEAT)
  {
    image = g_strdup_printf ("%d", _score);
  }
  else
  {
    image = g_strdup (" ");
  }

  return image;
}

// --------------------------------------------------------------------------------
const gchar *Score::GetStatusImage ()
{
  if (_status == DEFEAT)
  {
    return "D";
  }
  else if (_status == VICTORY)
  {
    return "V";
  }
  else if (_status == WITHDRAWAL)
  {
    return "A";
  }
  else if (_status == BLACK_CARD)
  {
    return "E";
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void Score::Clean ()
{
  _status = UNKNOWN;
}

// --------------------------------------------------------------------------------
void Score::Drop (gchar *reason)
{
  if (reason)
  {
    Status previous_status = _status;

    if (reason[0] == 'A')
    {
      _status = WITHDRAWAL;
    }
    else if (reason[0] == 'F')
    {
      _status = WITHDRAWAL;
    }
    else if (reason[0] == 'E')
    {
      _status = BLACK_CARD;
    }
    else
    {
      return;
    }

    if ((previous_status != WITHDRAWAL) && (previous_status != BLACK_CARD))
    {
      _backup_status = previous_status;
    }
  }
}

// --------------------------------------------------------------------------------
gchar Score::GetDropReason ()
{
  if (_status == WITHDRAWAL)
  {
    return 'A';
  }
  else if (_status == BLACK_CARD)
  {
    return 'E';
  }

  return 0;
}

// --------------------------------------------------------------------------------
void Score::Restore ()
{
  _status = _backup_status;
}

// --------------------------------------------------------------------------------
void Score::Set (guint    score,
                 gboolean victory)
{
  _score  = score;
  _status = DEFEAT;

  if (_score == _max->_value)
  {
    _status = VICTORY;
  }
  if (victory)
  {
    _status = VICTORY;
  }
}

// --------------------------------------------------------------------------------
gboolean Score::IsValid ()
{
  if (   (_status != UNKNOWN)
      && (_score > _max->_value))
  {
    return FALSE;
  }

  return TRUE;
}

// --------------------------------------------------------------------------------
gboolean Score::IsOut ()
{
  return ((_status == WITHDRAWAL) || (_status == BLACK_CARD));
}

// --------------------------------------------------------------------------------
void Score::SynchronizeWith (Score *with)
{
  if (IsOut () == FALSE)
  {
    if (with->IsOut ())
    {
      _backup_status = _status;
      _status        = OPPONENT_OUT;
    }
    else
    {
      _status = _backup_status;
    }
  }
}

// --------------------------------------------------------------------------------
gboolean Score::IsConsistentWith (Score *with)
{
  if ((_status == UNKNOWN) || (with->_status == UNKNOWN))
  {
    return TRUE;
  }
  else if ((_status == OPPONENT_OUT) || (with->_status == OPPONENT_OUT))
  {
    return TRUE;
  }
  else if ((_status == VICTORY) && (with->_status == VICTORY))
  {
    return FALSE;
  }
  else if ((_status == DEFEAT) && (with->_status == DEFEAT))
  {
    return FALSE;
  }
  else if ((_status == VICTORY) && (_score < with->_score))
  {
    return FALSE;
  }
  else if ((with->_status == VICTORY) && (with->_score < _score))
  {
    return FALSE;
  }
  else if (   (_score >= _max->_value)
           && (with->_score >= _max->_value))
  {
    return FALSE;
  }

  return TRUE;
}
