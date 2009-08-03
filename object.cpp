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

#include "object.hpp"

// --------------------------------------------------------------------------------
Object_c::Object_c ()
{
  g_datalist_init (&_datalist);
  _ref_count = 1;
}

// --------------------------------------------------------------------------------
Object_c::~Object_c ()
{
  if (_datalist)
  {
    g_datalist_clear (&_datalist);
  }
}

// --------------------------------------------------------------------------------
void Object_c::Retain ()
{
  _ref_count++;
}

// --------------------------------------------------------------------------------
void Object_c::Release ()
{
  if (_ref_count == 0)
  {
    g_print ("ERROR\n");
    return;
  }

  if (_ref_count == 1)
  {
    delete this;
    return;
  }
  else
  {
    _ref_count--;
  }
}

// --------------------------------------------------------------------------------
void Object_c::Release (Object_c *object)
{
  if (object)
  {
    object->Release ();
  }
}

// --------------------------------------------------------------------------------
void Object_c::SetData (gchar          *key,
                        void           *data,
                        GDestroyNotify  destroy_cbk)
{
  g_datalist_set_data_full (&_datalist,
                            key,
                            data,
                            destroy_cbk);
}

// --------------------------------------------------------------------------------
void *Object_c::GetData (gchar *key)
{
  return g_datalist_get_data (&_datalist,
                              key);
}
