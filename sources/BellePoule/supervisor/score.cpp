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

#include "util/data.hpp"

#include "score.hpp"

// --------------------------------------------------------------------------------
Score::Score  (Data     *max,
               gboolean  overflow_allowed)
  : Object ("Score")
{
  _max              = max;
  _score            = 0;
  _overflow_allowed = overflow_allowed;
  _status           = Status::UNKNOWN;
  _backup_status    = Status::UNKNOWN;
}

// --------------------------------------------------------------------------------
Score::~Score ()
{
}

// --------------------------------------------------------------------------------
gboolean Score::IsKnown ()
{
  return _status != Status::UNKNOWN;
}

// --------------------------------------------------------------------------------
gboolean Score::IsTheBest ()
{
  return _status == Status::VICTORY;
}

// --------------------------------------------------------------------------------
guint Score::Get ()
{
  if ((_status == Status::VICTORY) || (_status == Status::DEFEAT))
  {
    return _score;
  }

  return 0;
}

// --------------------------------------------------------------------------------
gchar *Score::GetImage ()
{
  gchar *image;

  if (_status == Status::UNKNOWN)
  {
    image = g_new0 (char, 1); // empty string
  }
  else if (_status == Status::VICTORY)
  {
    if ((_score == _max->GetValue ()) && (_overflow_allowed == FALSE))
    {
      image = g_strdup_printf ("V");
    }
    else
    {
      image = g_strdup_printf ("V%d", _score);
    }
  }
  else if (_status == Status::DEFEAT)
  {
    image = g_strdup_printf ("%d", _score);
  }
  else
  {
    image = g_strdup ("");
  }

  return image;
}

// --------------------------------------------------------------------------------
const gchar *Score::GetStatusImage ()
{
  if (_status == Status::DEFEAT)
  {
    return "D";
  }
  else if (_status == Status::VICTORY)
  {
    return "V";
  }
  else if (_status == Status::WITHDRAWAL)
  {
    return "A";
  }
  else if (_status == Status::BLACK_CARD)
  {
    return "E";
  }

  return nullptr;
}

// --------------------------------------------------------------------------------
void Score::Clean ()
{
  _status = Status::UNKNOWN;
}

// --------------------------------------------------------------------------------
void Score::Drop (gchar *reason)
{
  if (reason)
  {
    Status previous_status = _status;

    if (reason[0] == 'A')
    {
      _status = Status::WITHDRAWAL;
    }
    else if (reason[0] == 'F')
    {
      _status = Status::WITHDRAWAL;
    }
    else if (reason[0] == 'E')
    {
      _status = Status::BLACK_CARD;
    }
    else
    {
      return;
    }

    if ((previous_status != Status::WITHDRAWAL) && (previous_status != Status::BLACK_CARD))
    {
      _backup_status = previous_status;
    }
  }
}

// --------------------------------------------------------------------------------
gchar Score::GetDropReason ()
{
  if (_status == Status::WITHDRAWAL)
  {
    return 'A';
  }
  else if (_status == Status::BLACK_CARD)
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
  _status = Status::DEFEAT;

  if (_score == _max->GetValue ())
    {
      _status = Status::VICTORY;
    }
  if (victory)
  {
    _status = Status::VICTORY;
  }
}

// --------------------------------------------------------------------------------
gboolean Score::IsValid ()
{
  if (_status != Status::UNKNOWN)
  {
    if (_overflow_allowed)
    {
      return TRUE;
    }

    return (_score <= _max->GetValue ());
  }

  return TRUE;
}

// --------------------------------------------------------------------------------
gboolean Score::IsOut ()
{
  return ((_status == Status::WITHDRAWAL) || (_status == Status::BLACK_CARD));
}

// --------------------------------------------------------------------------------
void Score::SynchronizeWith (Score *with)
{
  if (IsOut () == FALSE)
  {
    if (with->IsOut ())
    {
      _backup_status = _status;
      _status        = Status::OPPONENT_OUT;
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
  if ((_status == Status::UNKNOWN) || (with->_status == Status::UNKNOWN))
  {
    return TRUE;
  }
  else if ((_status == Status::OPPONENT_OUT) || (with->_status == Status::OPPONENT_OUT))
  {
    return TRUE;
  }
  else if ((_status == Status::VICTORY) && (with->_status == Status::VICTORY))
  {
    return FALSE;
  }
  else if ((_status == Status::DEFEAT) && (with->_status == Status::DEFEAT))
  {
    return FALSE;
  }
  else if ((_status == Status::VICTORY) && (_score < with->_score))
  {
    return FALSE;
  }
  else if ((with->_status == Status::VICTORY) && (with->_score < _score))
  {
    return FALSE;
  }
  else if (   (_overflow_allowed == FALSE)
           && (_score       >= _max->GetValue ())
           && (with->_score >= _max->GetValue ()))
  {
    return FALSE;
  }

  return TRUE;
}
