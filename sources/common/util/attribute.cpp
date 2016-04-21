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

#include <stdlib.h>

#include "attribute.hpp"

// --------------------------------------------------------------------------------
Attribute::Attribute (AttributeDesc *desc)
: Object ("Attribute")
{
  _desc = desc;
  if (_desc)
  {
    _desc->Retain ();
  }
}

// --------------------------------------------------------------------------------
Attribute::~Attribute ()
{
  Object::TryToRelease (_desc);
}

// --------------------------------------------------------------------------------
Attribute *Attribute::New (const gchar *name)
{
  AttributeDesc *desc = AttributeDesc::GetDescFromCodeName (name);

  if (desc)
  {
    if (desc->_type == G_TYPE_STRING)
    {
      return new TextAttribute (desc);
    }
    else if (desc->_type == G_TYPE_BOOLEAN)
    {
      return new BooleanAttribute (desc);
    }
    else if (   (desc->_type == G_TYPE_INT)
             || (desc->_type == G_TYPE_UINT))
    {
      return new IntAttribute (desc);
    }
    else if (desc->_type == G_TYPE_ENUM)
    {
      return new TextAttribute (desc);
    }
  }
  return NULL;
}

// --------------------------------------------------------------------------------
gchar *Attribute::GetCodeName ()
{
  return _desc->_code_name;
}

// --------------------------------------------------------------------------------
gchar *Attribute::GetXmlName ()
{
  return _desc->_xml_name;
}

// --------------------------------------------------------------------------------
AttributeDesc *Attribute::GetDesc ()
{
  return _desc;
}

// --------------------------------------------------------------------------------
GType Attribute::GetType ()
{
  return _desc->_type;
}

// --------------------------------------------------------------------------------
gint Attribute::Compare (Attribute *a, Attribute *b)
{
  if (a)
  {
    if (a->_desc->_compare_func)
    {
      return a->_desc->_compare_func (a, b);
    }
    else
    {
      return a->CompareWith (b);
    }
  }

  return G_MAXINT;
}

// --------------------------------------------------------------------------------
char *Attribute::GetStrValue ()
{
  return NULL;
}

// --------------------------------------------------------------------------------
gint Attribute::GetIntValue ()
{
  return 0;
}

// --------------------------------------------------------------------------------
guint Attribute::GetUIntValue ()
{
  return 0;
}

// --------------------------------------------------------------------------------
TextAttribute::TextAttribute (AttributeDesc *desc)
  : Attribute (desc)
{
  _value = g_strdup ("");
}

// --------------------------------------------------------------------------------
TextAttribute::~TextAttribute ()
{
  g_free (_value);
}

// --------------------------------------------------------------------------------
void TextAttribute::SetValue (const gchar *value)
{
  g_free (_value);
  _value = NULL;

  if (value)
  {
    _value = GetUndivadableText (value);
  }
}

// --------------------------------------------------------------------------------
void TextAttribute::SetValue (guint value)
{
  g_free (_value);
  _value = g_strdup_printf ("%d", value);
}

// --------------------------------------------------------------------------------
char *TextAttribute::GetStrValue ()
{
  return _value;
}

// --------------------------------------------------------------------------------
gboolean TextAttribute::EntryIsTextBased ()
{
  return TRUE;
}

// --------------------------------------------------------------------------------
gchar *TextAttribute::GetXmlImage ()
{
  return g_strdup (_value);
}

// --------------------------------------------------------------------------------
void TextAttribute::TreeStoreSet (GtkTreeStore        *store,
                                  GtkTreeIter         *iter,
                                  gint                 column,
                                  AttributeDesc::Look  look)
{
  if (look == AttributeDesc::GRAPHICAL)
  {
    gtk_tree_store_set (store, iter,
                        column, GetPixbuf (),
                        -1);
  }
  else
  {
    gchar *value = GetUserImage (look);

    gtk_tree_store_set (store, iter,
                        column, value,
                        -1);
    g_free (value);
  }
}

// --------------------------------------------------------------------------------
gchar *TextAttribute::GetUserImage (AttributeDesc::Look look)
{
  if (look == AttributeDesc::GRAPHICAL)
  {
    // Fallback
    look = AttributeDesc::SHORT_TEXT;
  }

  if (_desc->HasDiscreteValue ())
  {
    return _desc->GetUserImage (_value,
                                look);
  }
  else
  {
    if (look == AttributeDesc::SHORT_TEXT)
    {
      return g_strndup (_value,
                        3);
    }
    else
    {
      return g_strdup (_value);
    }
  }
}

// --------------------------------------------------------------------------------
GdkPixbuf *TextAttribute::GetPixbuf ()
{
  return _desc->GetDiscretePixbuf (_value);
}

// --------------------------------------------------------------------------------
gint TextAttribute::CompareWith (Attribute *with)
{
  if (with)
  {
    return CompareWith (with->GetStrValue ());
  }

  return -1;
}

// --------------------------------------------------------------------------------
gint TextAttribute::CompareWith (const gchar *with)
{
  return (g_strcmp0 ((const gchar *) GetStrValue (), with));
}

// --------------------------------------------------------------------------------
Attribute *TextAttribute::Duplicate ()
{
  TextAttribute *attr = new TextAttribute (_desc);

  if (_value)
  {
    attr->_value = g_strdup (_value);
  }
  else
  {
    attr->_value = NULL;
  }

  return attr;
}

// --------------------------------------------------------------------------------
BooleanAttribute::BooleanAttribute (AttributeDesc *desc)
  : Attribute (desc)
{
  _value = FALSE;
}

// --------------------------------------------------------------------------------
BooleanAttribute::~BooleanAttribute ()
{
}

// --------------------------------------------------------------------------------
void BooleanAttribute::SetValue (const gchar *value)
{
  if (value)
  {
    _value = atoi (value);
  }
}

// --------------------------------------------------------------------------------
void BooleanAttribute::SetValue (guint value)
{
  _value = value;
}

// --------------------------------------------------------------------------------
guint BooleanAttribute::GetUIntValue ()
{
  return (guint) _value;
}

// --------------------------------------------------------------------------------
gboolean BooleanAttribute::EntryIsTextBased ()
{
  return FALSE;
}

// --------------------------------------------------------------------------------
gchar *BooleanAttribute::GetXmlImage ()
{
  return GetUserImage (AttributeDesc::LONG_TEXT);
}

// --------------------------------------------------------------------------------
GdkPixbuf *BooleanAttribute::GetPixbuf ()
{
  return _desc->GetDiscretePixbuf (_value);
}

// --------------------------------------------------------------------------------
void BooleanAttribute::TreeStoreSet (GtkTreeStore        *store,
                                     GtkTreeIter         *iter,
                                     gint                 column,
                                     AttributeDesc::Look  look)
{
  gtk_tree_store_set (store, iter,
                      column, _value,
                      -1);
}

// --------------------------------------------------------------------------------
gchar *BooleanAttribute::GetUserImage (AttributeDesc::Look look)
{
  return g_strdup_printf ("%d", _value);
}

// --------------------------------------------------------------------------------
gint BooleanAttribute::CompareWith (Attribute *with)
{
  if (with)
  {
    return (GetUIntValue () - with->GetUIntValue ());
  }

  return -1;
}

// --------------------------------------------------------------------------------
gint BooleanAttribute::CompareWith (const gchar *with)
{
  if (with)
  {
    return (GetUIntValue () - atoi (with));
  }

  return -1;
}

// --------------------------------------------------------------------------------
Attribute *BooleanAttribute::Duplicate ()
{
  BooleanAttribute *attr = new BooleanAttribute (_desc);

  attr->_value = _value;

  return attr;
}

// --------------------------------------------------------------------------------
IntAttribute::IntAttribute (AttributeDesc *desc)
  : Attribute (desc)
{
  _value = 0;
}

// --------------------------------------------------------------------------------
IntAttribute::~IntAttribute ()
{
}

// --------------------------------------------------------------------------------
void IntAttribute::SetValue (const gchar *value)
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
void IntAttribute::SetValue (guint value)
{
  _value = value;
}

// --------------------------------------------------------------------------------
gint IntAttribute::GetIntValue ()
{
  return (gint) _value;
}

// --------------------------------------------------------------------------------
guint IntAttribute::GetUIntValue ()
{
  return _value;
}

// --------------------------------------------------------------------------------
gboolean IntAttribute::EntryIsTextBased ()
{
  return TRUE;
}

// --------------------------------------------------------------------------------
gchar *IntAttribute::GetXmlImage ()
{
  return GetUserImage (AttributeDesc::LONG_TEXT);
}

// --------------------------------------------------------------------------------
void IntAttribute::TreeStoreSet (GtkTreeStore        *store,
                                 GtkTreeIter         *iter,
                                 gint                 column,
                                 AttributeDesc::Look  look)
{
  gtk_tree_store_set (store, iter,
                      column, _value,
                      -1);
}

// --------------------------------------------------------------------------------
gchar *IntAttribute::GetUserImage (AttributeDesc::Look look)
{
  if (_desc->_type == G_TYPE_UINT)
  {
    return g_strdup_printf ("%u", _value);
  }
  else
  {
    return g_strdup_printf ("%d", _value);
  }
}

// --------------------------------------------------------------------------------
GdkPixbuf *IntAttribute::GetPixbuf ()
{
  return _desc->GetDiscretePixbuf (_value);
}

// --------------------------------------------------------------------------------
gint IntAttribute::CompareWith (Attribute *with)
{
  if (with)
  {
    return (GetUIntValue () - with->GetUIntValue ());
  }

  return G_MININT;
}

// --------------------------------------------------------------------------------
gint IntAttribute::CompareWith (const gchar *with)
{
  if (with)
  {
    return (GetUIntValue () - atoi (with));
  }

  return G_MININT;
}

// --------------------------------------------------------------------------------
Attribute *IntAttribute::Duplicate ()
{
  IntAttribute *attr = new IntAttribute (_desc);

  attr->_value = _value;

  return attr;
}
