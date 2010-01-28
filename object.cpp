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

#include <string.h>

#include "object.hpp"

guint Object_c::_nb_objects = 0;
GList *Object_c::_list      = NULL;

struct ClassStatus
{
  gchar *_name;
  guint  _nb_objects;
};

// --------------------------------------------------------------------------------
Object_c::Object_c (gchar *class_name)
{
  g_datalist_init (&_datalist);
  _ref_count = 1;
  _nb_objects++;

#ifdef DEBUG
  if (class_name == NULL)
  {
    class_name = "anonymous";
  }

  _class_name = class_name;

  for (guint i = 0; i < g_list_length (_list); i++)
  {
    ClassStatus *status;

    status = (ClassStatus *) g_list_nth_data (_list,
                                              i);

    if (strcmp (status->_name, class_name) == 0)
    {
      status->_nb_objects++;
      return;
    }
  }

  {
    ClassStatus *new_status = (ClassStatus *) new ClassStatus;

    new_status->_name       = class_name;
    new_status->_nb_objects = 1;
    _list = g_list_append (_list,
                           new_status);
  }
#endif
}

// --------------------------------------------------------------------------------
Object_c::~Object_c ()
{
  if (_datalist)
  {
    g_datalist_clear (&_datalist);
  }
  _nb_objects--;

#ifdef DEBUG
  for (guint i = 0; i < g_list_length (_list); i++)
  {
    ClassStatus *status;

    status = (ClassStatus *) g_list_nth_data (_list,
                                              i);

    if (strcmp (status->_name, _class_name) == 0)
    {
      status->_nb_objects--;
      return;
    }
  }
#endif
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
void Object_c::TryToRelease (Object_c *object)
{
  if (object)
  {
    object->Release ();
  }
}

// --------------------------------------------------------------------------------
void Object_c::SetData (Object_c       *owner,
                        gchar          *key,
                        void           *data,
                        GDestroyNotify  destroy_cbk)
{
  gchar *full_key = g_strdup_printf ("%p::%s", owner, key);

  g_datalist_set_data_full (&_datalist,
                            full_key,
                            data,
                            destroy_cbk);
  g_free (full_key);
}

// --------------------------------------------------------------------------------
void *Object_c::GetData (Object_c *owner,
                         gchar    *key)
{
  void  *data;
  gchar *full_key = g_strdup_printf ("%p::%s", owner, key);

  data = g_datalist_get_data (&_datalist,
                              full_key);
  g_free (full_key);

  return data;
}

// --------------------------------------------------------------------------------
void Object_c::Dump ()
{
  guint total = 0;

  g_mem_profile ();

  g_print (">> %d\n", _nb_objects);
  for (guint i = 0; i < g_list_length (_list); i++)
  {
    ClassStatus *status;

    status = (ClassStatus *) g_list_nth_data (_list,
                                              i);

    g_print ("%20s ==> %d\n", status->_name,
                              status->_nb_objects);
    total += status->_nb_objects;
  }
  g_print ("TOTAL ==> %d\n", total);

}
