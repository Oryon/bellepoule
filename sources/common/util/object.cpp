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

#include "network/message.hpp"

#include "flash_code.hpp"
#include "object.hpp"

#ifdef DEBUG
guint        Object::_nb_objects     = 0;
GList       *Object::_list           = NULL;
const gchar *Object::_klass_to_track = NULL;

struct ClassStatus
{
  const gchar *_name;
  guint        _nb_objects;
};
#endif

// --------------------------------------------------------------------------------
Object::Object (const gchar *class_name)
{
  g_datalist_init (&_datalist);
  _ref_count = 1;

  _flash_code    = NULL;
  _parcel        = NULL;
  _listener_list = NULL;

#ifdef DEBUG
  _nb_objects++;

  if (class_name == NULL)
  {
    _class_name = "anonymous";
  }
  else
  {
    _class_name = class_name;
  }

  if (_klass_to_track && g_strcmp0 (_klass_to_track, _class_name) == 0)
  {
    printf ("%s creation\n    ", _class_name);
    Dump ();
  }

  {
    GList *current = _list;

    while (current)
    {
      ClassStatus *status = (ClassStatus *) current->data;

      if (g_strcmp0 (status->_name, _class_name) == 0)
      {
        status->_nb_objects++;
        return;
      }

      current = g_list_next (current);
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
  {
    GList *current = _listener_list;

    while (current)
    {
      Listener *listener = (Listener *) current->data;

      listener->OnObjectDeleted (this);

      current = g_list_next (current);
    }
    g_list_free (_listener_list);
  }

  if (_datalist)
  {
    g_datalist_clear (&_datalist);
  }

  TryToRelease (_flash_code);

  if (_parcel)
  {
    Recall  ();
    _parcel->Release ();
  }

#ifdef DEBUG
  _nb_objects--;

  if (_klass_to_track && g_strcmp0 (_klass_to_track, _class_name) == 0)
  {
    printf ("%s destruction\n    ", _class_name);
    Dump ();
  }

  for (guint i = 0; i < g_list_length (_list); i++)
  {
    ClassStatus *status;

    status = (ClassStatus *) g_list_nth_data (_list,
                                              i);

    if (g_strcmp0 (status->_name, _class_name) == 0)
    {
      status->_nb_objects--;
      return;
    }
  }

#endif
}

// --------------------------------------------------------------------------------
gchar *Object::GetUndivadableText (const gchar *text)
{
#ifdef WIN32
  return g_strdup (text);
#else
  guint8 *result = NULL;

  if (text)
  {
    guint  nb_space = 0;

    for (gchar *current = (gchar *) text; *current != 0; current++)
    {
      if (*current == ' ')
      {
        nb_space++;
      }
    }

    result = (guint8 *) g_malloc (strlen (text) + nb_space*2 +1);

    {
      guint8 *current = result;

      for (guint i = 0; text[i] != 0; i++)
      {
        if (text[i] == ' ')
        {
          // non breaking space
          *current = 0xC2;
          current++;
          *current = 0xA0;
        }
        else
        {
          *current = text[i];
        }
        current++;
      }
      *current = 0;
    }
  }

  return (gchar *) result;
#endif
}

// --------------------------------------------------------------------------------
void Object::SetFlashRef (const gchar *ref)
{
  _flash_code = new FlashCode (ref);
}

// --------------------------------------------------------------------------------
FlashCode *Object::GetFlashCode ()
{
  return _flash_code;
}

// --------------------------------------------------------------------------------
void Object::AddObjectListener (Listener *listener)
{
  _listener_list = g_list_prepend (_listener_list,
                                   listener);
}

// --------------------------------------------------------------------------------
void Object::RemoveObjectListener (Listener *listener)
{
  GList *node = g_list_find (_listener_list,
                             listener);

  if (node)
  {
    _listener_list = g_list_remove_link (_listener_list,
                                         node);
  }
}

// --------------------------------------------------------------------------------
void Object::Retain ()
{
  _ref_count++;
#ifdef DEBUG
  if (_klass_to_track && g_strcmp0 (_klass_to_track, _class_name) == 0)
  {
    printf ("%s retained\n    ", _class_name);
    Dump ();
  }
#endif
}

// --------------------------------------------------------------------------------
void Object::Release ()
{
#ifdef DEBUG
  if (_klass_to_track && g_strcmp0 (_klass_to_track, _class_name) == 0)
  {
    printf ("%s released\n    ", _class_name);
    Dump ();
  }
#endif

  if (_ref_count == 0)
  {
    g_warning ("Object::Release ERROR\n");
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
void Object::Disclose (const gchar *as)
{
  if (_parcel == NULL)
  {
    _parcel = new Net::Message (as);
  }
}

// --------------------------------------------------------------------------------
void Object::Recall ()
{
  if (_parcel)
  {
    _parcel->Recall ();
  }
}

// --------------------------------------------------------------------------------
void Object::Spread ()
{
  if (_parcel)
  {
    FeedParcel (_parcel);
    _parcel->Spread ();
  }
}

// --------------------------------------------------------------------------------
void Object::FeedParcel (Net::Message *parcel)
{
}

// --------------------------------------------------------------------------------
gboolean Object::OnMessage (Net::Message *message)
{
  return FALSE;
}

// --------------------------------------------------------------------------------
void Object::Dump ()
{
#ifdef DEBUG
  printf ("%p ==> %s\n", (void *) this, _class_name);
#endif
}

// --------------------------------------------------------------------------------
void Object::DumpList ()
{
#ifdef DEBUG
  guint total = 0;

  g_mem_profile ();

  for (guint i = 0; i < g_list_length (_list); i++)
  {
    ClassStatus *status;

    status = (ClassStatus *) g_list_nth_data (_list,
                                              i);

    g_print ("%20s ==> %d\n", status->_name,
                              status->_nb_objects);
    total += status->_nb_objects;
  }
  g_print ("TOTAL ==> " BLUE "%d\n" ESC, total);
#endif
}

// --------------------------------------------------------------------------------
void Object::Track (const gchar *klass)
{
#ifdef DEBUG
  _klass_to_track = klass;
#endif
}
