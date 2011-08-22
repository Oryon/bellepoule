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
static gchar *GetUndivadableText (const gchar *text)
{
  gchar *result = NULL;

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

    result = (gchar *) g_malloc (strlen (text) + nb_space*2 +1);

    {
      gchar *current = result;

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

  return result;
}

// --------------------------------------------------------------------------------
AttributeDesc::AttributeDesc (GType        type,
                              const gchar *code_name,
                              const gchar *xml_name,
                              const gchar *user_name)
: Object ("AttributeDesc")
{
  _type                  = type;
  _code_name             = g_strdup (code_name);
  _xml_name              = g_strdup (xml_name);
  _user_name             = g_strdup (user_name);
  _uniqueness            = SINGULAR;
  _persistency           = PERSISTENT;
  _scope                 = GLOBAL;
  _free_value_allowed    = TRUE;
  _rights                = PUBLIC;
  _discrete_model        = NULL;
  _compare_func          = NULL;
  _has_selector          = FALSE;
  _is_selector           = FALSE;
}

// --------------------------------------------------------------------------------
AttributeDesc::~AttributeDesc ()
{
  g_slist_foreach (_list,
                   (GFunc) Object::TryToRelease,
                   NULL);
  g_slist_free (_list);

  if (_discrete_model)
  {
    g_object_unref (_discrete_model);
  }
  g_free (_code_name);
  g_free (_xml_name);
  g_free (_user_name);
}

// --------------------------------------------------------------------------------
const gchar *AttributeDesc::GetTranslation (const gchar *domain,
                                            const gchar *text)
{
  if (domain)
  {
    return (dgettext (domain,
                      text));
  }
  else
  {
    return text;
  }
}

// --------------------------------------------------------------------------------
AttributeDesc *AttributeDesc::Declare (GType        type,
                                       const gchar *code_name,
                                       const gchar *xml_name,
                                       gchar       *user_name)
{
  AttributeDesc *attr_desc = new AttributeDesc (type,
                                                code_name,
                                                xml_name,
                                                user_name);

  _list = g_slist_append (_list,
                          attr_desc);

  return attr_desc;
}

// --------------------------------------------------------------------------------
gboolean AttributeDesc::DiscreteFilterForCombobox (GtkTreeModel *model,
                                                   GtkTreeIter  *iter,
                                                   GtkComboBox  *selector)
{
  gchar    *country;
  gchar    *selector_country = NULL;
  gboolean  result           = FALSE;

  gtk_tree_model_get (model, iter,
                      AttributeDesc::DISCRETE_SELECTOR, &country,
                      -1);

  {
    GtkTreeIter selector_iter;

    if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (selector),
                                       &selector_iter))
    {
      GtkTreeModel *selector_model = gtk_combo_box_get_model (selector);

      gtk_tree_model_get (selector_model, &selector_iter,
                          DISCRETE_XML_IMAGE, &selector_country, -1);
    }
  }

  if (selector_country)
  {
    if (country && (g_ascii_strcasecmp (selector_country, country) == 0))
    {
      result = TRUE;
    }

    g_free (selector_country);
  }

  return result;
}

// --------------------------------------------------------------------------------
void AttributeDesc::BindDiscreteValues (GObject         *object,
                                        GtkCellRenderer *renderer,
                                        GtkComboBox     *selector)
{
  if (_discrete_model)
  {
    if (_has_selector)
    {
      GtkTreeModel *filter = gtk_tree_model_filter_new (_discrete_model,
                                                        NULL);

      gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter),
                                              (GtkTreeModelFilterVisibleFunc) DiscreteFilterForCombobox,
                                              selector,
                                              NULL);

      if (selector)
      {
        GSList *selector_clients = (GSList *) g_object_get_data (G_OBJECT (selector), "selector_clients");

        selector_clients = g_slist_prepend (selector_clients,
                                            filter);
        g_object_set_data (G_OBJECT (selector),
                           "selector_clients", selector_clients);
      }
      g_object_set (object,
                    "model", filter,
                    NULL);
      g_object_unref (filter);
    }
    else
    {
      g_object_set (object,
                    "model", _discrete_model,
                    NULL);
    }

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
void AttributeDesc::BindDiscreteValues (GtkCellRenderer *renderer)
{
  if (_discrete_model)
  {
    g_object_set (G_OBJECT (renderer),
                  "model",       _discrete_model,
                  "text-column", DISCRETE_USER_IMAGE,
                  NULL);
  }
}

// --------------------------------------------------------------------------------
void AttributeDesc::Refilter (GtkComboBox *selector,
                              void        *data)
{
  GSList *selector_clients = (GSList *) g_object_get_data (G_OBJECT (selector), "selector_clients");

  while (selector_clients)
  {
    GtkTreeModel *model = (GtkTreeModel *) selector_clients->data;

    gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (model));
    selector_clients = g_slist_next (selector_clients);
  }
}

// --------------------------------------------------------------------------------
gboolean AttributeDesc::HasDiscreteValue ()
{
  return (_discrete_model != NULL);
}

// --------------------------------------------------------------------------------
const gchar *AttributeDesc::GetDiscreteXmlImage (const gchar *from_user_image)
{
  if (_discrete_model)
  {
    GtkTreeIter iter;
    gboolean    iter_is_valid;

    iter_is_valid = gtk_tree_model_get_iter_first (_discrete_model,
                                                   &iter);
    while (iter_is_valid)
    {
      gchar *current_image;
      gchar *xml_image;

      gtk_tree_model_get (_discrete_model,     &iter,
                          DISCRETE_USER_IMAGE, (void *) &current_image,
                          DISCRETE_XML_IMAGE,  (void *) &xml_image,
                          -1);
      if (strcmp (current_image, from_user_image) == 0)
      {
        return xml_image;
      }
      iter_is_valid = gtk_tree_model_iter_next (_discrete_model,
                                                &iter);
    }
  }
  return from_user_image;
}

// --------------------------------------------------------------------------------
gchar *AttributeDesc::GetDiscreteUserImage (guint from_code)
{
  gchar *image = (gchar *) GetDiscreteData (from_code,
                                            DISCRETE_USER_IMAGE);
  if (image)
  {
    return g_strdup (image);
  }
  else
  {
    return NULL;
  }
}

// --------------------------------------------------------------------------------
gchar *AttributeDesc::GetDiscreteIcon (guint from_code)
{
  return (gchar *) GetDiscreteData (from_code,
                                    DISCRETE_ICON_NAME);
}

// --------------------------------------------------------------------------------
void *AttributeDesc::GetDiscreteData (guint from_code,
                                      guint column)
{
  if (_discrete_model)
  {
    GtkTreeIter iter;
    gboolean    iter_is_valid;

    iter_is_valid = gtk_tree_model_get_iter_first (_discrete_model,
                                                   &iter);
    while (iter_is_valid)
    {
      guint  current_code;
      void  *data;

      gtk_tree_model_get (_discrete_model, &iter,
                          DISCRETE_CODE, &current_code,
                          column,        &data,
                          -1);
      if (current_code == from_code)
      {
        return data;
      }
      iter_is_valid = gtk_tree_model_iter_next (_discrete_model,
                                                &iter);
    }
  }

  return NULL;
}

// --------------------------------------------------------------------------------
gchar *AttributeDesc::GetXmlImage (gchar *user_image)
{
  if (_discrete_model)
  {
    GtkTreeIter iter;
    gboolean    iter_is_valid;

    iter_is_valid = gtk_tree_model_get_iter_first (_discrete_model,
                                                   &iter);
    while (iter_is_valid)
    {
      gchar *current_xml_image;
      gchar *current_user_image;

      gtk_tree_model_get (_discrete_model, &iter,
                          DISCRETE_XML_IMAGE, &current_xml_image,
                          DISCRETE_USER_IMAGE, &current_user_image,
                          -1);
      if (strcmp (current_user_image, user_image) == 0)
      {
        return g_strdup (current_xml_image);
      }
      iter_is_valid = gtk_tree_model_iter_next (_discrete_model,
                                                &iter);
    }
  }

  return NULL;
}

// --------------------------------------------------------------------------------
gchar *AttributeDesc::GetUserImage (GtkTreeIter *iter)
{
  if (_discrete_model)
  {
    gchar *image;

    gtk_tree_model_get (_discrete_model, iter,
                        DISCRETE_USER_IMAGE, &image, -1);
    if (image)
    {
      return g_strdup (image);
    }
  }

  return NULL;
}

// --------------------------------------------------------------------------------
gchar *AttributeDesc::GetXmlImage (GtkTreeIter *iter)
{
  if (_discrete_model)
  {
    gchar *image;

    gtk_tree_model_get (_discrete_model, iter,
                        DISCRETE_XML_IMAGE, &image, -1);
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
    if (_discrete_model)
    {
      GtkTreeIter iter;
      gboolean    iter_is_valid;

      iter_is_valid = gtk_tree_model_get_iter_first (_discrete_model,
                                                     &iter);
      while (iter_is_valid)
      {
        gchar *current_xml_image;
        gchar *current_user_image;

        gtk_tree_model_get (_discrete_model, &iter,
                            DISCRETE_XML_IMAGE, &current_xml_image,
                            DISCRETE_USER_IMAGE, &current_user_image,
                            -1);
        if (strcmp (current_xml_image, xml_image) == 0)
        {
          return g_strdup (current_user_image);
        }
        iter_is_valid = gtk_tree_model_iter_next (_discrete_model,
                                                  &iter);
      }
    }

    return GetUndivadableText (xml_image);
  }
  else
  {
    return NULL;
  }
}

// --------------------------------------------------------------------------------
void AttributeDesc::AddDiscreteValues (const gchar *first_xml_image,
                                       gchar       *first_user_image,
                                       const gchar *first_icon,
                                       ...)
{
  if (_discrete_model == NULL)
  {
    _discrete_model = GTK_TREE_MODEL (gtk_tree_store_new (6, G_TYPE_UINT,
                                                          G_TYPE_STRING,
                                                          G_TYPE_STRING,
                                                          GDK_TYPE_PIXBUF,
                                                          G_TYPE_STRING,
                                                          G_TYPE_STRING));
  }

  {
    va_list  ap;
    gchar   *xml_image  = g_strdup (first_xml_image);
    gchar   *user_image = first_user_image;
    gchar   *icon       = g_strdup (first_icon);

    va_start (ap, first_icon);
    while (xml_image)
    {
      GtkTreeIter  iter;
      gchar       *undivadable_image;

      gtk_tree_store_append (GTK_TREE_STORE (_discrete_model), &iter, NULL);

      undivadable_image = GetUndivadableText (user_image);
      gtk_tree_store_set (GTK_TREE_STORE (_discrete_model), &iter,
                          DISCRETE_CODE, xml_image[0],
                          DISCRETE_XML_IMAGE, xml_image,
                          DISCRETE_USER_IMAGE, undivadable_image, -1);
      if (*icon)
      {
        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (icon, NULL);

        gtk_tree_store_set (GTK_TREE_STORE (_discrete_model), &iter,
                            DISCRETE_ICON_NAME, icon,
                            DISCRETE_ICON,      pixbuf, -1);
      }
      g_free (undivadable_image);

      xml_image  = va_arg (ap, char *);
      if (xml_image)
      {
        user_image = va_arg (ap, char *);
        icon       = va_arg (ap, char *);
      }
    }
    va_end (ap);
  }
}

// --------------------------------------------------------------------------------
void AttributeDesc::AddDiscreteValueSelector (const gchar *file)
{
  gchar *path = g_build_filename (_program_path, "resources", file, NULL);

  _discrete_model = GTK_TREE_MODEL (gtk_tree_store_new (6, G_TYPE_UINT,
                                                        G_TYPE_STRING,
                                                        G_TYPE_STRING,
                                                        GDK_TYPE_PIXBUF,
                                                        G_TYPE_STRING,
                                                        G_TYPE_STRING));

  _is_selector = TRUE;

  AddDiscreteValues (path,
                     "");

  g_free (path);
}

// --------------------------------------------------------------------------------
void AttributeDesc::AddDiscreteValues (const gchar *file)
{
  gchar        *dir_path = g_build_filename (_program_path, "resources", "data", NULL);
  GDir         *dir      = g_dir_open       (dir_path, 0,  NULL);
  const gchar  *country  = g_dir_read_name  (dir);

  _discrete_model = GTK_TREE_MODEL (gtk_tree_store_new (6, G_TYPE_UINT,
                                                        G_TYPE_STRING,
                                                        G_TYPE_STRING,
                                                        GDK_TYPE_PIXBUF,
                                                        G_TYPE_STRING,
                                                        G_TYPE_STRING));

  _has_selector = TRUE;

  while (country)
  {
    gchar  *full_path = g_build_filename (dir_path, country, file, NULL);

    AddDiscreteValues (full_path,
                       country);

    g_free (full_path);
    country = g_dir_read_name (dir);
  }

  g_dir_close (dir);
  g_free (dir_path);
}

// --------------------------------------------------------------------------------
void AttributeDesc::AddDiscreteValues (const gchar *file,
                                       const gchar *selector)
{
  gchar  *raw_file;
  GError *error = NULL;

  if (g_file_get_contents ((const gchar *) file,
                           &raw_file,
                           NULL,
                           &error) == FALSE)
  {
    g_print ("AttributeDesc::AddDiscreteValues -> %s\n", error->message);
    g_free (error);
  }
  else
  {
    guint nb_tokens = 1;

    for (gchar *current = raw_file; current && (*current != '\n'); current++)
    {
      if (*current == ';')
      {
        nb_tokens++;
      }
    }

    if (nb_tokens == 4)
    {
      gchar **tokens = g_strsplit_set (raw_file,
                                       ";\n",
                                       0);

      if (tokens)
      {
        gchar *textdomain = NULL;

        {
          gchar *dirname = g_path_get_dirname (file);

          if (dirname)
          {
            gchar *translations = g_build_filename (dirname, "translations", NULL);

            if (g_file_test (translations, G_FILE_TEST_IS_DIR))
            {
              gchar *sufix;

              textdomain = g_path_get_basename (file);

              sufix = strchr (textdomain, '.');
              if (sufix)
              {
                *sufix = 0;
              }
            }

            g_free (dirname);
          }
        }

        for (guint i = 0; (tokens[i] != NULL) && (*tokens[i] != 0); i+=nb_tokens)
        {
          GtkTreeIter iter;

          gtk_tree_store_append (GTK_TREE_STORE (_discrete_model), &iter, NULL);

          // Selector
          {
            gtk_tree_store_set (GTK_TREE_STORE (_discrete_model), &iter,
                                DISCRETE_SELECTOR, selector,
                                -1);
          }

          {
            gchar *undivadable_text;

            // XML code
            {
              undivadable_text = GetUndivadableText (tokens[i]);
              gtk_tree_store_set (GTK_TREE_STORE (_discrete_model), &iter,
                                  DISCRETE_XML_IMAGE, undivadable_text,
                                  -1);
            }

            // Short name
            {
              if (*tokens[i+1] != 0)
              {
                g_free (undivadable_text);
                undivadable_text = GetUndivadableText (GetTranslation (textdomain,
                                                                       tokens[i+1]));
              }
            }

            // Long name
            {
              if (*tokens[i+2] != 0)
              {
                g_free (undivadable_text);
                undivadable_text = GetUndivadableText (GetTranslation (textdomain,
                                                                       tokens[i+2]));
              }

              gtk_tree_store_set (GTK_TREE_STORE (_discrete_model), &iter,
                                  DISCRETE_USER_IMAGE, undivadable_text,
                                  -1);
            }

            g_free (undivadable_text);
          }

          // Internal code (French federation issue)
          {
            gtk_tree_store_set (GTK_TREE_STORE (_discrete_model), &iter,
                                DISCRETE_CODE, tokens[i+3],
                                -1);
          }
        }
        g_strfreev (tokens);
        g_free (textdomain);
      }
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
      if (strcmp (name, desc->_code_name) == 0)
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
AttributeDesc *AttributeDesc::GetDesc (const gchar *name)
{
  for (guint i = 0; i < g_slist_length (_list); i++)
  {
    AttributeDesc *attr_desc;

    attr_desc = (AttributeDesc *) g_slist_nth_data (_list,
                                                    i);
    if (strcmp (attr_desc->_code_name, name) == 0)
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
  g_print ("**** Wrong call to GetStrValue ****\n");
  return NULL;
}

// --------------------------------------------------------------------------------
gint Attribute::GetIntValue ()
{
  g_print ("**** Wrong call to GetIntValue ****\n");
  return 0;
}

// --------------------------------------------------------------------------------
guint Attribute::GetUIntValue ()
{
  g_print ("**** Wrong call to GetUIntValue ****\n");
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
  if (value)
  {
    g_free (_value);
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
  return true;
}

// --------------------------------------------------------------------------------
gchar *TextAttribute::GetXmlImage ()
{
  return g_strdup (_value);
}

// --------------------------------------------------------------------------------
void *TextAttribute::GetListStoreValue ()
{
  return GetUserImage ();
}

// --------------------------------------------------------------------------------
gchar *TextAttribute::GetUserImage ()
{
  if (_desc->HasDiscreteValue ())
  {
    return _desc->GetUserImage (_value);
  }
  else
  {
    return g_strdup (_value);
  }
}

// --------------------------------------------------------------------------------
gint TextAttribute::CompareWith (Attribute *with)
{
  if (with)
  {
    return (strcmp ((const gchar *) GetStrValue (), (const gchar *) with->GetStrValue ()));
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
  return false;
}

// --------------------------------------------------------------------------------
gchar *BooleanAttribute::GetXmlImage ()
{
  return GetUserImage ();
}

// --------------------------------------------------------------------------------
void *BooleanAttribute::GetListStoreValue ()
{
  return (void *) _value;
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
    return (GetUIntValue () - with->GetUIntValue ());
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
  return true;
}

// --------------------------------------------------------------------------------
gchar *IntAttribute::GetXmlImage ()
{
  return GetUserImage ();
}

// --------------------------------------------------------------------------------
void *IntAttribute::GetListStoreValue ()
{
  return (void *) _value;
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
    return (GetUIntValue () - with->GetUIntValue ());
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
