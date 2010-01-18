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

GSList *AttributeDesc::_list = NULL;

// --------------------------------------------------------------------------------
AttributeDesc::AttributeDesc (GType  type,
                              gchar *name)
: Object_c ("AttributeDesc")
{
  _type       = type;
  _name       = name;
  _uniqueness = SINGULAR;
  _rights     = PUBLIC;
}

// --------------------------------------------------------------------------------
AttributeDesc::~AttributeDesc ()
{
  for (guint i = 0; i < g_slist_length (_list); i++)
  {
    AttributeDesc *attr_desc;

    attr_desc = (AttributeDesc *) g_slist_nth_data (_list,
                                                    i);
    attr_desc->Release ();
  }
  g_slist_free (_list);
}

// --------------------------------------------------------------------------------
AttributeDesc *AttributeDesc::Declare (GType  type,
                                       gchar *name)
{
  AttributeDesc *attr = new AttributeDesc (type,
                                           name);

  _list = g_slist_append (_list,
                          attr);

  return attr;
}

// --------------------------------------------------------------------------------
void AttributeDesc::CreateList (GSList **list, ...)
{
  va_list  ap;
  gchar   *name;

  *list = g_slist_copy (_list);
  va_start (ap, list);
  while (name = va_arg (ap, char *))
  {
    for (guint i = 0; i < g_slist_length (*list); i++)
    {
      AttributeDesc *desc;

      desc = (AttributeDesc *) g_slist_nth_data (*list,
                                                 i);
      if (strcmp (name, desc->_name) == 0)
      {
        *list = g_slist_remove (*list,
                                desc);
        break;
      }
    }
  }
  va_end(ap);
}

// --------------------------------------------------------------------------------
AttributeDesc *AttributeDesc::GetDesc (gchar *name)
{
  for (guint i = 0; i < g_slist_length (_list); i++)
  {
    AttributeDesc *attr_desc;

    attr_desc = (AttributeDesc *) g_slist_nth_data (_list,
                                                    i);
    if (strcmp (attr_desc->_name, name) == 0)
    {
      return attr_desc;
    }
  }

  return NULL;
}




// --------------------------------------------------------------------------------
Attribute_c::Attribute_c (AttributeDesc *desc)
: Object_c ("Attribute_c")
{
  _desc = desc;
  if (_desc)
  {
    _desc->Retain ();
  }
}

// --------------------------------------------------------------------------------
Attribute_c::~Attribute_c ()
{
  Object_c::TryToRelease (_desc);
}

// --------------------------------------------------------------------------------
Attribute_c *Attribute_c::New (gchar *name)
{
  AttributeDesc *desc = AttributeDesc::GetDesc (name);

  if (desc)
  {
    if (desc->_type == G_TYPE_STRING)
    {
      return new TextAttribute_c (desc);
    }
    else if (desc->_type == G_TYPE_BOOLEAN)
    {
      return new BooleanAttribute_c (desc);
    }
    else if (desc->_type == G_TYPE_INT)
    {
      return new IntAttribute_c (desc);
    }
  }
  return NULL;
}

// --------------------------------------------------------------------------------
gchar *Attribute_c::GetName ()
{
  return _desc->_name;
}

// --------------------------------------------------------------------------------
GType Attribute_c::GetType ()
{
  return _desc->_type;
}

// --------------------------------------------------------------------------------
gint Attribute_c::Compare (Attribute_c *a, Attribute_c *b)
{
  if (a)
  {
    return a->CompareWith (b);
  }

  return G_MAXINT;
}

// --------------------------------------------------------------------------------
TextAttribute_c::TextAttribute_c (AttributeDesc *desc)
  : Attribute_c (desc)
{
  _value = g_strdup ("");
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
BooleanAttribute_c::BooleanAttribute_c (AttributeDesc *desc)
  : Attribute_c (desc)
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
IntAttribute_c::IntAttribute_c (AttributeDesc *desc)
  : Attribute_c (desc)
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

  return G_MININT;
}
