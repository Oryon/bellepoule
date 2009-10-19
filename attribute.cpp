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
#include <stdlib.h>

#include "attribute.hpp"

GSList *Attribute_c::_list = NULL;

// --------------------------------------------------------------------------------
Attribute_c::Attribute_c (gchar *name)
{
  if (name)
  {
    _name = g_strdup (name);
  }
  else
  {
    name = NULL;
  }
}

// --------------------------------------------------------------------------------
Attribute_c::~Attribute_c ()
{
  if (_name)
  {
    g_free (_name);
  }
}

// --------------------------------------------------------------------------------
void Attribute_c::Add (GType  type,
                       gchar *name)
{
  Desc *desc = new (Desc);

  desc->type = type;
  desc->name = g_strdup (name);

  _list = g_slist_append (_list,
                          desc);
}

// --------------------------------------------------------------------------------
Attribute_c::Desc *Attribute_c::GetDesc (gchar *name)
{
  if (_list)
  {
    for (guint i = 0; i < g_slist_length (_list); i++)
    {
      Desc *desc;

      desc = (Desc *) g_slist_nth_data (_list, i);
      if (strcmp (desc->name, name) == 0)
      {
        return desc;
      }
    }
  }
  return NULL;
}

// --------------------------------------------------------------------------------
Attribute_c *Attribute_c::New (gchar *name)
{
  Desc *desc = GetDesc (name);

  if (desc)
  {
    if (desc->type == G_TYPE_STRING)
    {
      return new TextAttribute_c (name);
    }
    else if (desc->type == G_TYPE_BOOLEAN)
    {
      return new BooleanAttribute_c (name);
    }
    else if (desc->type == G_TYPE_INT)
    {
      return new IntAttribute_c (name);
    }
  }
  return NULL;
}

// --------------------------------------------------------------------------------
guint Attribute_c::GetNbAttributes ()
{
  if (_list)
  {
    return g_slist_length (_list);
  }

  return 0;
}

// --------------------------------------------------------------------------------
gchar *Attribute_c::GetNthAttributeName (guint nth)
{
  if (_list)
  {
    Desc *desc = (Desc *) g_slist_nth_data (_list, nth);

    return desc->name;
  }

  return NULL;
}

// --------------------------------------------------------------------------------
GType Attribute_c::GetNthAttributeType (guint nth)
{
  if (_list)
  {
    Desc *desc = (Desc *) g_slist_nth_data (_list, nth);

    return desc->type;
  }

  return G_TYPE_INT;
}

// --------------------------------------------------------------------------------
GType Attribute_c::GetAttributeType (gchar *name)
{
  if (_list)
  {
    Desc *desc = GetDesc (name);

    if (desc)
    {
      return desc->type;
    }
  }

  return G_TYPE_INT;
}

// --------------------------------------------------------------------------------
guint Attribute_c::GetAttributeId (gchar *name)
{
  if (_list)
  {
    for (guint i = 0; i < g_slist_length (_list); i++)
    {
      Desc *desc;

      desc = (Desc *) g_slist_nth_data (_list, i);
      if (strcmp (desc->name, name) == 0)
      {
        return i;
      }
    }
  }

  return 0xFFFFFFFF;
}

// --------------------------------------------------------------------------------
gchar *Attribute_c::GetName ()
{
  return _name;
}

// --------------------------------------------------------------------------------
gint Attribute_c::Compare (Attribute_c *a, Attribute_c *b)
{
  if (a)
  {
    return a->CompareWith (b);
  }

  return -1;
}

// --------------------------------------------------------------------------------
TextAttribute_c::TextAttribute_c (gchar *name)
  : Attribute_c (name)
{
  _value = g_strdup ("??");
}

// --------------------------------------------------------------------------------
TextAttribute_c::~TextAttribute_c ()
{
  g_free (_value);
}

// --------------------------------------------------------------------------------
void TextAttribute_c::SetValue (gchar *value)
{
  if (value)
  {
    g_free (_value);
    _value = g_strdup (value);
  }
}

// --------------------------------------------------------------------------------
void TextAttribute_c::SetValue (guint value)
{
  g_free (_value);
  _value = g_strdup_printf ("%d", value);
}

// --------------------------------------------------------------------------------
void *TextAttribute_c::GetValue ()
{
  return _value;
}

// --------------------------------------------------------------------------------
gboolean TextAttribute_c::EntryIsTextBased ()
{
  return true;
}

// --------------------------------------------------------------------------------
gchar *TextAttribute_c::GetStringImage ()
{
  return g_strdup (_value);
}

// --------------------------------------------------------------------------------
gint TextAttribute_c::CompareWith (Attribute_c *with)
{
  if (with)
  {
    return (strcmp ((const gchar *) GetValue (), (const gchar *) with->GetValue ()));
  }

  return -1;
}

// --------------------------------------------------------------------------------
BooleanAttribute_c::BooleanAttribute_c (gchar *name)
  : Attribute_c (name)
{
  _value = false;
}

// --------------------------------------------------------------------------------
BooleanAttribute_c::~BooleanAttribute_c ()
{
}

// --------------------------------------------------------------------------------
void BooleanAttribute_c::SetValue (gchar *value)
{
  if (value)
  {
    _value = atoi (value);
  }
}

// --------------------------------------------------------------------------------
void BooleanAttribute_c::SetValue (guint value)
{
  _value = value;
}

// --------------------------------------------------------------------------------
void *BooleanAttribute_c::GetValue ()
{
  return (void *) _value;
}

// --------------------------------------------------------------------------------
gboolean BooleanAttribute_c::EntryIsTextBased ()
{
  return false;
}

// --------------------------------------------------------------------------------
gchar *BooleanAttribute_c::GetStringImage ()
{
  return g_strdup_printf ("%d", _value);
}

// --------------------------------------------------------------------------------
gint BooleanAttribute_c::CompareWith (Attribute_c *with)
{
  if (with)
  {
    return ((guint) GetValue () - (guint) with->GetValue ());
  }

  return -1;
}

// --------------------------------------------------------------------------------
IntAttribute_c::IntAttribute_c (gchar *name)
  : Attribute_c (name)
{
  _value = 0;
}

// --------------------------------------------------------------------------------
IntAttribute_c::~IntAttribute_c ()
{
}

// --------------------------------------------------------------------------------
void IntAttribute_c::SetValue (gchar *value)
{
  if (value)
  {
    if (strstr (value, "0x"))
    {
      _value = strtol (value, NULL, 16);
    }
    else
    {
      _value = strtol (value, NULL, 10);
    }
  }
}

// --------------------------------------------------------------------------------
void IntAttribute_c::SetValue (guint value)
{
  _value = value;
}

// --------------------------------------------------------------------------------
void *IntAttribute_c::GetValue ()
{
  return (void *) _value;
}

// --------------------------------------------------------------------------------
gboolean IntAttribute_c::EntryIsTextBased ()
{
  return true;
}

// --------------------------------------------------------------------------------
gchar *IntAttribute_c::GetStringImage ()
{
  return g_strdup_printf ("%d", _value);
}

// --------------------------------------------------------------------------------
gint IntAttribute_c::CompareWith (Attribute_c *with)
{
  if (with)
  {
    return ((guint) GetValue () - (guint) with->GetValue ());
  }

  return -1;
}
