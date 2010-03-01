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
Score_c::Score_c  (Data *max)
: Object_c ("Score_c")
{
  _score     = 0;
  _is_known  = false;
  _max       = max;
}

// --------------------------------------------------------------------------------
Score_c::~Score_c ()
{
}

// --------------------------------------------------------------------------------
gboolean Score_c::IsKnown ()
{
  return _is_known;
}

// --------------------------------------------------------------------------------
guint Score_c::Get ()
{
  return _score;
}

// --------------------------------------------------------------------------------
gchar *Score_c::GetImage ()
{
  gchar *image;

  if (_is_known)
  {
    if (_score == _max->_value)
    {
      image = g_strdup_printf ("V");
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
void Score_c::Clean ()
{
  _is_known = false;
}

// --------------------------------------------------------------------------------
void Score_c::Set (gint score)
{
  if (score < 0)
  {
    _is_known = false;
  }
  else
  {
    _is_known = true;
    _score    = score;
  }
}

// --------------------------------------------------------------------------------
gboolean Score_c::IsValid ()
{
  if (   IsKnown ()
      && (_score > _max->_value))
  {
    return false;
  }

  return true;
}

// --------------------------------------------------------------------------------
gboolean Score_c::IsConsistentWith (Score_c *with)
{
  if (   (IsKnown ()       == FALSE)
      || (with->IsKnown () == FALSE))
  {
    return TRUE;
  }
  else if (Get () == with->Get ())
  {
    return false;
  }
  else if (   (Get () >= _max->_value)
           && (with->Get () >= _max->_value))
  {
    return false;
  }

  return true;
}
