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

guint Object::_nb_objects = 0;
GList *Object::_list      = NULL;

struct ClassStatus
{
  const gchar *_name;
  guint        _nb_objects;
};

// --------------------------------------------------------------------------------
Object::Object (const gchar *class_name)
{
  g_datalist_init (&_datalist);
  _ref_count = 1;
  _nb_objects++;

#ifdef DEBUG
  if (class_name == NULL)
  {
    _class_name = g_strdup ("anonymous");
  }
  else
  {
    _class_name = g_strdup (class_name);
  }

  for (guint i = 0; i < g_list_length (_list); i++)
  {
    ClassStatus *status;

    status = (ClassStatus *) g_list_nth_data (_list,
                                              i);

    if (strcmp (status->_name, _class_name) == 0)
    {
      status->_nb_objects++;
      return;
    }
  }

  {
    ClassStatus *new_status = (ClassStatus *) new ClassStatus;

    new_status->_name       = _class_name;
    new_status->_nb_objects = 1;
    _list = g_list_append (_list,
                           new_status);
  }
#endif
}

// --------------------------------------------------------------------------------
Object::~Object ()
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

  g_free (_class_name);

#endif
}

// --------------------------------------------------------------------------------
void Object::Retain ()
{
  _ref_count++;
}

// --------------------------------------------------------------------------------
void Object::Release ()
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
void Object::TryToRelease (Object *object)
{
  if (object)
  {
    object->Release ();
  }
}

// --------------------------------------------------------------------------------
void Object::SetData (Object         *owner,
                      const gchar    *key,
                      void           *data,
                      GDestroyNotify  destroy_cbk)
{
  gchar *full_key = g_strdup_printf ("%p::%s", (void *) owner, key);

  g_datalist_set_data_full (&_datalist,
                            full_key,
                            data,
                            destroy_cbk);
  g_free (full_key);
}

// --------------------------------------------------------------------------------
void Object::RemoveData (Object      *owner,
                         const gchar *key)
{
  gchar *full_key = g_strdup_printf ("%p::%s", (void *) owner, key);

  g_datalist_remove_data (&_datalist,
                          full_key);
  g_free (full_key);
}

// --------------------------------------------------------------------------------
void Object::RemoveAllData ()
{
  if (_datalist)
  {
    g_datalist_clear (&_datalist);
    _datalist = NULL;
  }
}

// --------------------------------------------------------------------------------
void *Object::GetPtrData (Object      *owner,
                          const gchar *key)
{
  void  *data;
  gchar *full_key = g_strdup_printf ("%p::%s", (void *) owner, key);

  data = g_datalist_get_data (&_datalist,
                              full_key);
  g_free (full_key);

  return data;
}

// --------------------------------------------------------------------------------
guint Object::GetUIntData (Object      *owner,
                           const gchar *key)
{
  return GPOINTER_TO_UINT (GetPtrData (owner,
                                       key));
}

// --------------------------------------------------------------------------------
gint Object::GetIntData (Object      *owner,
                         const gchar *key)
{
  return GPOINTER_TO_INT (GetPtrData (owner,
                                      key));
}

// --------------------------------------------------------------------------------
void Object::Dump ()
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
