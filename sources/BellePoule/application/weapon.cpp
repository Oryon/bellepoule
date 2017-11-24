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

#include "weapon.hpp"

GList *Weapon::_list = NULL;

// --------------------------------------------------------------------------------
Weapon::Weapon (const gchar *image,
                const gchar *xml_image,
                const gchar *greg_image,
                guint        standard_duration_sec)
  : Object ("Weapon")
{
  _image      = g_strdup (image);
  _xml_image  = g_strdup (xml_image);
  _greg_image = g_strdup (greg_image);

  _standard_duration_sec = standard_duration_sec;

  _list = g_list_prepend (_list,
                          this);
}

// --------------------------------------------------------------------------------
Weapon::~Weapon ()
{
  GList *node = g_list_find (_list, this);

  if (node)
  {
    _list = g_list_delete_link (_list,
                                node);
  }

  g_free (_image);
  g_free (_xml_image);
}

// --------------------------------------------------------------------------------
guint Weapon::GetStandardDuration ()
{
  return _standard_duration_sec;
}

// --------------------------------------------------------------------------------
void Weapon::FreeList ()
{
  while (_list)
  {
    Weapon *weapon = (Weapon *) _list->data;

    weapon->Release ();
  }
}

// --------------------------------------------------------------------------------
GList *Weapon::GetList ()
{
  return _list;
}

// --------------------------------------------------------------------------------
Weapon *Weapon::GetFromXml (const gchar *xml)
{
  GList *current = _list;

  while (current)
  {
    Weapon *weapon = (Weapon *) current->data;

    if (g_strcmp0 (xml, weapon->_xml_image) == 0)
    {
      return weapon;
    }

    current = g_list_next (current);
  }

  return NULL;
}

// --------------------------------------------------------------------------------
guint Weapon::GetIndex ()
{
  return g_list_index (_list,
                       this);
}

// --------------------------------------------------------------------------------
Weapon *Weapon::GetFromIndex (guint index)
{
  return (Weapon *) g_list_nth_data (_list,
                                     index);
}

// --------------------------------------------------------------------------------
Weapon *Weapon::GetDefault ()
{
  if (_list)
  {
    return (Weapon *) _list->data;
  }

  return NULL;
}

// --------------------------------------------------------------------------------
const gchar *Weapon::GetImage ()
{
  return _image;
}

// --------------------------------------------------------------------------------
const gchar *Weapon::GetXmlImage ()
{
  return _xml_image;
}

// --------------------------------------------------------------------------------
const gchar *Weapon::GetGregImage ()
{
  return _greg_image;
}
