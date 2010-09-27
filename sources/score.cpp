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
  _score       = 0;
  _is_known    = FALSE;
  _max         = max;
  _is_the_best = FALSE;
}

// --------------------------------------------------------------------------------
Score::~Score ()
{
}

// --------------------------------------------------------------------------------
gboolean Score::IsKnown ()
{
  return _is_known;
}

// --------------------------------------------------------------------------------
gboolean Score::IsTheBest ()
{
  return _is_the_best;
}

// --------------------------------------------------------------------------------
guint Score::Get ()
{
  return _score;
}

// --------------------------------------------------------------------------------
gchar *Score::GetImage ()
{
  gchar *image;

  if (_is_known)
  {
    if (_score == _max->_value)
    {
      image = g_strdup_printf ("V");
    }
    else if (_is_the_best)
    {
      image = g_strdup_printf ("V%d", _score);
    }
    else
    {
      image = g_strdup_printf ("%d", _score);
    }
  }
  else
  {
    image = g_strdup ("");
  }

  return image;
}

// --------------------------------------------------------------------------------
void Score::Clean ()
{
  _is_known = FALSE;
}

// --------------------------------------------------------------------------------
void Score::Set (gint     score,
                 gboolean is_the_best)
{
  if (score < 0)
  {
    _is_known    = FALSE;
    _is_the_best = FALSE;
  }
  else
  {
    _is_known    = TRUE;
    _score       = score;
    _is_the_best = is_the_best;
    if (_score == _max->_value)
    {
      _is_the_best = TRUE;
    }
  }
}

// --------------------------------------------------------------------------------
gboolean Score::IsValid ()
{
  if (   IsKnown ()
      && (_score > _max->_value))
  {
    return FALSE;
  }

  return TRUE;
}

// --------------------------------------------------------------------------------
gboolean Score::IsConsistentWith (Score *with)
{
  if (   (IsKnown ()       == FALSE)
      || (with->IsKnown () == FALSE))
  {
    return TRUE;
  }
  else if (_is_the_best == with->_is_the_best)
  {
    return FALSE;
  }
  else if (_is_the_best && _score < with->_score)
  {
    return FALSE;
  }
  else if (with->_is_the_best && with->_score < _score)
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
