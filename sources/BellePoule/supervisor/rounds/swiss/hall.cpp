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

#include "hall.hpp"

namespace Swiss
{
  // --------------------------------------------------------------------------------
  Hall::Hall ()
    : Object ("Swiss::Hall")
  {
    _piste_count    = 0;
    _table_size     = 0;
    _piste_register = nullptr;
  }

  // --------------------------------------------------------------------------------
  Hall::~Hall ()
  {
    g_free (_piste_register);
  }

  // --------------------------------------------------------------------------------
  void Hall::Dump ()
  {
    for (guint i = 0; i < _piste_count; i++)
    {
      printf ("%d ==> %d\n", i+1, _piste_register[i]);
    }
    printf ("\n");
  }

  // --------------------------------------------------------------------------------
  void Hall::Clear ()
  {
    for (guint i = 0; i < _table_size; i++)
    {
      _piste_register[i] = FALSE;
    }
  }

  // --------------------------------------------------------------------------------
  void Hall::SetPisteCount (guint count)
  {
    if (count > _table_size)
    {
      _table_size = count;
      _piste_register = (gboolean *) g_realloc (_piste_register,
                                                _table_size);

      for (guint i = _piste_count; i < _table_size; i++)
      {
        _piste_register[i] = FALSE;
      }
    }

    _piste_count = count;
  }

  // --------------------------------------------------------------------------------
  void Hall::BookPiste (guint piste)
  {
    if ((0 < piste) && (piste <= _piste_count))
    {
      _piste_register[piste-1] = TRUE;
    }
  }

  // --------------------------------------------------------------------------------
  guint Hall::BookPiste ()
  {
    for (guint i = 0; i < _piste_count; i++)
    {
      if (_piste_register[i] == FALSE)
      {
        _piste_register[i] = TRUE;
        return i+1;
      }
    }
    return 0;
  }

  // --------------------------------------------------------------------------------
  void Hall::Free (guint piste)
  {
    if ((0 < piste) && (piste <= _table_size))
    {
      _piste_register[piste-1] = FALSE;
    }
  }
}
