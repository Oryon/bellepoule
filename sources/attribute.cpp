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

typedef enum
{
  DISCRETE_XML_IMAGE,
  DISCRETE_USER_IMAGE
} DiscreteColumnId;

GSList *AttributeDesc::_list = NULL;

// --------------------------------------------------------------------------------
AttributeDesc::AttributeDesc (GType        type,
                              gchar       *xml_name,
                              gchar       *user_name)
: Object ("AttributeDesc")
{
  _type               = type;
  _xml_name           = xml_name;
  _user_name          = user_name;
  _uniqueness         = SINGULAR;
  _persistency        = PERSISTENT;
  _scope              = GLOBAL;
  _free_value_allowed = TRUE;
  _rights             = PUBLIC;
  _discrete_store     = NULL;
  _compare_func       = NULL;
}

// --------------------------------------------------------------------------------
AttributeDesc::~AttributeDesc ()
{
  g_slist_foreach (_list,
                   (GFunc) Object::TryToRelease,
                   NULL);
  g_slist_free (_list);

  if (_discrete_store)
  {
    g_object_unref (_discrete_store);
  }
}

// --------------------------------------------------------------------------------
AttributeDesc *AttributeDesc::Declare (GType  type,
                                       gchar *xml_name,
                                       gchar *user_name)
{
  AttributeDesc *attr_desc = new AttributeDesc (type,
                                                xml_name,
                                                user_name);

  _list = g_slist_append (_list,
                          attr_desc);

  return attr_desc;
}

// --------------------------------------------------------------------------------
void AttributeDesc::BindDiscreteValues (GObject         *object,
                                        GtkCellRenderer *renderer)
{
  if (_discrete_store)
  {
    g_object_set (object,
                  "model", _discrete_store,
                  NULL);
    if (renderer)
    {
      gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (object), renderer,
                                      "text", DISCRETE_USER_IMAGE, NULL);
    }
    else
    {
      g_object_set (object,
                    "text-column", DISCRETE_USER_IMAGE,
                    NULL);
    }
  }
}

// --------------------------------------------------------------------------------
gboolean AttributeDesc::HasDiscreteValue ()
{
  return (_discrete_store != NULL);
}

// --------------------------------------------------------------------------------
gchar *AttributeDesc::GetXmlImage (gchar *user_image)
{
  if (_discrete_store)
  {
    GtkTreeIter iter;
    gboolean    iter_is_valid;

    iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_discrete_store),
                                                   &iter);
    while (iter_is_valid)
    {
      gchar *current_xml_image;
      gchar *current_user_image;

      gtk_tree_model_get (GTK_TREE_MODEL (_discrete_store), &iter,
                          DISCRETE_XML_IMAGE, &current_xml_image,
                          DISCRETE_USER_IMAGE, &current_user_image,
                          -1);
      if (strcmp (current_user_image, user_image) == 0)
      {
        return g_strdup (current_xml_image);
      }
      iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_discrete_store),
                                                &iter);
    }
  }

  return NULL;
}

// --------------------------------------------------------------------------------
gchar *AttributeDesc::GetUserImage (GtkTreeIter *iter)
{
  if (_discrete_store)
  {
    gchar *image;

    gtk_tree_model_get (GTK_TREE_MODEL (_discrete_store), iter,
                        DISCRETE_USER_IMAGE, &image, -1);
    if (image)
    {
      return g_strdup (image);
    }
  }

  return NULL;
}

// --------------------------------------------------------------------------------
gchar *AttributeDesc::GetUserImage (gchar *xml_image)
{
  if (xml_image)
  {
    if (_discrete_store)
    {
      GtkTreeIter iter;
      gboolean    iter_is_valid;

      iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_discrete_store),
                                                     &iter);
      while (iter_is_valid)
      {
        gchar *current_xml_image;
        gchar *current_user_image;

        gtk_tree_model_get (GTK_TREE_MODEL (_discrete_store), &iter,
                            DISCRETE_XML_IMAGE, &current_xml_image,
                            DISCRETE_USER_IMAGE, &current_user_image,
                            -1);
        if (strcmp (current_xml_image, xml_image) == 0)
        {
          return g_strdup (current_user_image);
        }
        iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_discrete_store),
                                                  &iter);
      }
    }

    return g_strdup (xml_image);
  }
  else
  {
    return NULL;
  }
}

// --------------------------------------------------------------------------------
void AttributeDesc::AddDiscreteValues (gchar *first_xml_image,
                                       gchar *first_user_image,
                                       ...)
{
  if (_discrete_store == NULL)
  {
    _discrete_store = gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
  }

  {
    va_list  ap;
    gchar   *xml_image  = first_xml_image;
    gchar   *user_image = first_user_image;

    va_start (ap, first_user_image);
    while (xml_image)
    {
      GtkTreeIter iter;

      gtk_tree_store_append (_discrete_store, &iter, NULL);

      gtk_tree_store_set (_discrete_store, &iter,
                          DISCRETE_XML_IMAGE, xml_image,
                          DISCRETE_USER_IMAGE, user_image, -1);

      xml_image  = va_arg (ap, char *);
      if (xml_image)
      {
        user_image = va_arg (ap, char *);
      }
    }
    va_end (ap);
  }
}

// --------------------------------------------------------------------------------
void AttributeDesc::AddDiscreteValues (gchar *file)
{
  if (_discrete_store == NULL)
  {
    _discrete_store = gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
  }

  {
    gchar *raw_file;
    gchar **tokens;

    g_file_get_contents ((const gchar *) file,
                         &raw_file,
                         NULL,
                         NULL);

    tokens = g_strsplit_set (raw_file,
                             ";\n",
                             0);

    if (tokens)
    {
      for (guint i = 0; (tokens[i] != NULL) && (*tokens[i] != 0); i+=2)
      {
        GtkTreeIter iter;

        gtk_tree_store_append (_discrete_store, &iter, NULL);

        gtk_tree_store_set (_discrete_store, &iter,
                            DISCRETE_XML_IMAGE, tokens[i],
                            DISCRETE_USER_IMAGE, tokens[i+1], -1);
      }
      g_strfreev (tokens);
    }
  }
}

// --------------------------------------------------------------------------------
void AttributeDesc::CreateList (GSList **list, ...)
{
  va_list  ap;
  gchar   *name;

  *list = g_slist_copy (_list);

  va_start (ap, list);
  while ( (name = va_arg (ap, char *)) )
  {
    for (guint i = 0; i < g_slist_length (*list); i++)
    {
      AttributeDesc *desc;

      desc = (AttributeDesc *) g_slist_nth_data (*list,
                                                 i);
      if (strcmp (name, desc->_xml_name) == 0)
      {
        *list = g_slist_remove (*list,
                                desc);
        break;
      }
    }
  }
  va_end (ap);
}

// --------------------------------------------------------------------------------
GSList *AttributeDesc::GetList ()
{
  return _list;
}

// --------------------------------------------------------------------------------
AttributeDesc *AttributeDesc::GetDesc (gchar *name)
{
  for (guint i = 0; i < g_slist_length (_list); i++)
  {
    AttributeDesc *attr_desc;

    attr_desc = (AttributeDesc *) g_slist_nth_data (_list,
                                                    i);
    if (strcmp (attr_desc->_xml_name, name) == 0)
    {
      return attr_desc;
    }
  }

  return NULL;
}




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
Attribute *Attribute::New (gchar *name)
{
  AttributeDesc *desc = AttributeDesc::GetDesc (name);

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
    else if (desc->_type == G_TYPE_INT)
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
gchar *Attribute::GetName ()
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
void TextAttribute::SetValue (gchar *value)
{
  if (value)
  {
    g_free (_value);
    _value = g_strdup (value);
  }
}

// --------------------------------------------------------------------------------
void TextAttribute::SetValue (guint value)
{
  g_free (_value);
  _value = g_strdup_printf ("%d", value);
}

// --------------------------------------------------------------------------------
void *TextAttribute::GetValue ()
{
  return _value;
}

// --------------------------------------------------------------------------------
gboolean TextAttribute::EntryIsTextBased ()
{
  return true;
}

// --------------------------------------------------------------------------------
gchar *TextAttribute::GetXmlImage ()
{
  gchar *image = _desc->GetXmlImage (_value);

  if (image == NULL)
  {
    image = GetUserImage ();
  }

  return image;
}

// --------------------------------------------------------------------------------
gchar *TextAttribute::GetUserImage ()
{
  return g_strdup (_value);
}

// --------------------------------------------------------------------------------
gint TextAttribute::CompareWith (Attribute *with)
{
  if (with)
  {
    return (strcmp ((const gchar *) GetValue (), (const gchar *) with->GetValue ()));
  }

  return -1;
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
  _value = false;
}

// --------------------------------------------------------------------------------
BooleanAttribute::~BooleanAttribute ()
{
}

// --------------------------------------------------------------------------------
void BooleanAttribute::SetValue (gchar *value)
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
void *BooleanAttribute::GetValue ()
{
  return (void *) _value;
}

// --------------------------------------------------------------------------------
gboolean BooleanAttribute::EntryIsTextBased ()
{
  return false;
}

// --------------------------------------------------------------------------------
gchar *BooleanAttribute::GetXmlImage ()
{
  return GetUserImage ();
}

// --------------------------------------------------------------------------------
gchar *BooleanAttribute::GetUserImage ()
{
  return g_strdup_printf ("%d", _value);
}

// --------------------------------------------------------------------------------
gint BooleanAttribute::CompareWith (Attribute *with)
{
  if (with)
  {
    return ((guint) GetValue () - (guint) with->GetValue ());
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
void IntAttribute::SetValue (gchar *value)
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
void *IntAttribute::GetValue ()
{
  return (void *) _value;
}

// --------------------------------------------------------------------------------
gboolean IntAttribute::EntryIsTextBased ()
{
  return true;
}

// --------------------------------------------------------------------------------
gchar *IntAttribute::GetXmlImage ()
{
  return GetUserImage ();
}

// --------------------------------------------------------------------------------
gchar *IntAttribute::GetUserImage ()
{
  return g_strdup_printf ("%d", _value);
}

// --------------------------------------------------------------------------------
gint IntAttribute::CompareWith (Attribute *with)
{
  if (with)
  {
    return ((guint) GetValue () - (guint) with->GetValue ());
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
