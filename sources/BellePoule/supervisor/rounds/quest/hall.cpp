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

#include "../../match.hpp"
#include "hall.hpp"

namespace Quest
{
  // --------------------------------------------------------------------------------
  Hall::Hall ()
    : Object ("Quest::Hall")
  {
    _piste_count = 0;
    _table_size  = 0;
    _owners      = nullptr;
  }

  // --------------------------------------------------------------------------------
  Hall::~Hall ()
  {
    g_free (_owners);
  }

  // --------------------------------------------------------------------------------
  void Hall::Dump ()
  {
    for (guint i = 0; i < _piste_count; i++)
    {
      if (_owners[i])
      {
        printf ("%d ==> %s\n", i+1, _owners[i]->GetName ());
      }
      else
      {
        printf ("%d ==> nullptr\n", i+1);
      }
    }
    printf ("\n");
  }

  // --------------------------------------------------------------------------------
  void Hall::Clear ()
  {
    for (guint i = 0; i < _table_size; i++)
    {
      _owners[i] = nullptr;
    }
  }

  // --------------------------------------------------------------------------------
  void Hall::SetPisteCount (guint count)
  {
    if (count > _table_size)
    {
      _table_size = count;
      _owners = (Match **) g_realloc (_owners,
                                      _table_size * sizeof (Match *));

      for (guint i = _piste_count; i < _table_size; i++)
      {
        _owners[i] = nullptr;
      }
    }

    _piste_count = count;
  }

  // --------------------------------------------------------------------------------
  guint Hall::BookPiste (Match *owner)
  {
    guint piste = owner->GetPiste ();

    if (piste == 0)
    {
      for (guint i = 0; i < _piste_count; i++)
      {
        if (_owners[i] == nullptr)
        {
          _owners[i] = owner;
          owner->SetPiste (i+1);
          break;
        }
      }
    }
    else if (piste <= _piste_count)
    {
      _owners[piste-1] = owner;
      owner->SetPiste (piste);
    }

    return owner->GetPiste ();
  }

  // --------------------------------------------------------------------------------
  void Hall::FreePiste (Match *owner)
  {
    guint piste = owner->GetPiste ();

    if ((0 < piste) && (piste <= _table_size))
    {
      if (_owners[piste-1] == owner)
      {
        _owners[piste-1] = nullptr;
      }
    }
  }
}
