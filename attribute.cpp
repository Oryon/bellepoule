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
  _name = name;
}

// --------------------------------------------------------------------------------
Attribute_c::~Attribute_c ()
{
}

// --------------------------------------------------------------------------------
void Attribute_c::Add (GType  type,
                       gchar *name)
{
  Desc *desc = new (Desc);

  desc->type        = type;
  desc->name        = g_strdup (name);
  desc->client_list = NULL;

  _list = g_slist_append (_list,
                          desc);
}

// --------------------------------------------------------------------------------
Attribute_c::Desc *Attribute_c::GetDesc (gchar *name)
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
  return NULL;
}

// --------------------------------------------------------------------------------
void Attribute_c::AddClient (gchar    *name,
                             Object_c *client)
{
  Desc *desc = GetDesc (name);

  if (desc)
  {
    desc->client_list = g_slist_append (desc->client_list,
                                        client);
  }
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
guint Attribute_c::GetNbAttributes (Object_c *client)
{
  if (_list)
  {
    if (client == NULL)
    {
      return g_slist_length (_list);
    }
    else
    {
      guint nb = 0;

      for (guint i = 0; i < g_slist_length (_list); i++)
      {
        Desc *desc;

        desc = (Desc *) g_slist_nth_data (_list, i);
        if (g_slist_find (desc->client_list, client))
        {
          nb++;
        }
      }
      return nb;
    }
  }

  return 0;
}

// --------------------------------------------------------------------------------
gchar *Attribute_c::GetNthAttributeName (guint     nth,
                                         Object_c *client)
{
  if (_list)
  {
    if (client == NULL)
    {
      Desc *desc = (Desc *) g_slist_nth_data (_list, nth);

      return desc->name;
    }
    else
    {
      guint nb = 0;

      for (guint i = 0; i < g_slist_length (_list); i++)
      {
        Desc *desc;

        desc = (Desc *) g_slist_nth_data (_list, i);
        if (g_slist_find (desc->client_list, client))
        {
          if (nb == nth)
          {
            return desc->name;
          }
          nb++;
        }
      }
    }
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void Attribute_c::GetNthAttribute (guint      nth,
                                   gchar    **name,
                                   GType     *type,
                                   Object_c  *client)
{
  *name = NULL;
  *type = G_TYPE_INT;

  if (_list)
  {
    if (client == NULL)
    {
      Desc *desc = (Desc *) g_slist_nth_data (_list, nth);

      *name = desc->name;
      *type = desc->type;
    }
    else
    {
      guint nb = 0;

      for (guint i = 0; i < g_slist_length (_list); i++)
      {
        Desc *desc;

        desc = (Desc *) g_slist_nth_data (_list, i);
        if (g_slist_find (desc->client_list, client))
        {
          if (nb == nth)
          {
            *name = desc->name;
            *type = desc->type;
            return;
          }
          nb++;
        }
      }
    }
  }
}

// --------------------------------------------------------------------------------
gchar *Attribute_c::GetName ()
{
  return _name;
}

// --------------------------------------------------------------------------------
gint Attribute_c::Compare (Attribute_c *a, Attribute_c *b)
{
  return a->CompareWith (b);
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
  return (strcmp ((const gchar *) GetValue (), (const gchar *) with->GetValue ()));
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
  return ((guint) GetValue () - (guint) with->GetValue ());
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
  return ((guint) GetValue () - (guint) with->GetValue ());
}
