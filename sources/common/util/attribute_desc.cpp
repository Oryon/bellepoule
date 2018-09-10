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

#include "global.hpp"

#include "attribute_desc.hpp"
#include "tree_model_index.hpp"

GList                       *AttributeDesc::_list           = NULL;
GSList                      *AttributeDesc::_swappable_list = NULL;
AttributeDesc::CriteriaFunc  AttributeDesc::_criteria_func  = NULL;

// --------------------------------------------------------------------------------
AttributeDesc::AttributeDesc (GType        type,
                              const gchar *code_name,
                              const gchar *xml_name,
                              const gchar *user_name)
: Object ("AttributeDesc")
{
  _type               = type;
  _code_name          = g_strdup (code_name);
  _xml_name           = g_strdup (xml_name);
  _user_name          = g_strdup (user_name);
  _uniqueness         = SINGULAR;
  _favorite_look      = LONG_TEXT;
  _persistency        = PERSISTENT;
  _scope              = GLOBAL;
  _free_value_allowed = TRUE;
  _rights             = PUBLIC;
  _discrete_model     = NULL;
  _compare_func       = NULL;
  _has_selector       = FALSE;
  _is_selector        = FALSE;

  _discrete_code_index       = NULL;
  _discrete_xml_index        = NULL;
  _discrete_long_text_index  = NULL;
  _discrete_short_text_index = NULL;
}

// --------------------------------------------------------------------------------
AttributeDesc::~AttributeDesc ()
{
  if (_discrete_model)
  {
    _discrete_code_index->Release       ();
    _discrete_xml_index->Release        ();
    _discrete_long_text_index->Release  ();
    _discrete_short_text_index->Release ();

    g_object_unref (_discrete_model);
  }

  g_free (_code_name);
  g_free (_xml_name);
  g_free (_user_name);
}

// --------------------------------------------------------------------------------
void AttributeDesc::SetCriteria (const gchar  *criteria,
                                 CriteriaFunc  func)
{
  _criteria_func = func;
}

// --------------------------------------------------------------------------------
gboolean AttributeDesc::MatchCriteria (const gchar *criteria)
{
  if (_criteria_func)
  {
    return _criteria_func (this);
  }
  return TRUE;
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

  _list = g_list_append (_list,
                         attr_desc);

  return attr_desc;
}

// --------------------------------------------------------------------------------
void AttributeDesc::Cleanup ()
{
  FreeFullGList (AttributeDesc, _list);
}

// --------------------------------------------------------------------------------
void AttributeDesc::AddSwappable (AttributeDesc *desc)
{
  _swappable_list = g_slist_append (_swappable_list,
                                    desc);
}

// --------------------------------------------------------------------------------
GSList *AttributeDesc::GetSwappableList ()
{
  return _swappable_list;
}

// --------------------------------------------------------------------------------
gboolean AttributeDesc::DiscreteFilterForCombobox (GtkTreeModel *model,
                                                   GtkTreeIter  *iter,
                                                   GtkComboBox  *selector)
{
  gchar    *selector_country = NULL;
  gboolean  result           = FALSE;

  {
    GtkTreeIter selector_iter;

    if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (selector),
                                       &selector_iter))
    {
      GtkTreeModel *selector_model = gtk_combo_box_get_model (selector);

      gtk_tree_model_get (selector_model, &selector_iter,
                          DISCRETE_XML_IMAGE_str, &selector_country, -1);
    }
  }

  if (selector_country)
  {
    gchar *country;

    gtk_tree_model_get (model, iter,
                        DISCRETE_SELECTOR_str, &country,
                        -1);

    if (country)
    {
      result = (g_ascii_strcasecmp (selector_country, country) == 0);
      g_free (country);
    }
    g_free (selector_country);
  }

  return result;
}

// --------------------------------------------------------------------------------
void AttributeDesc::BindDiscreteValues (GObject     *object,
                                        GtkComboBox *selector)
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
  }
}

// --------------------------------------------------------------------------------
gint AttributeDesc::CompareTextColumn (GtkTreeModel *model,
                                       GtkTreeIter  *a,
                                       GtkTreeIter  *b,
                                       gpointer      user_data)
{
  gboolean  result;
  gchar    *a_image;
  gchar    *b_image;

  gtk_tree_model_get (model, a,
                      DISCRETE_XML_IMAGE_str, (void *) &a_image,
                      -1);
  gtk_tree_model_get (model, b,
                      DISCRETE_XML_IMAGE_str, (void *) &b_image,
                      -1);
  result = g_strcmp0 (a_image, b_image);

  g_free (a_image);
  g_free (b_image);

  return result;
}

// --------------------------------------------------------------------------------
void AttributeDesc::EnableSorting ()
{
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (_discrete_model),
                                   DISCRETE_LONG_TEXT_str,
                                   (GtkTreeIterCompareFunc) CompareTextColumn,
                                   NULL,
                                   NULL);

  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (_discrete_model),
                                        DISCRETE_LONG_TEXT_str,
                                        GTK_SORT_ASCENDING);
}

// --------------------------------------------------------------------------------
void AttributeDesc::TreeStoreSetDefault (GtkTreeStore        *store,
                                         GtkTreeIter         *iter,
                                         gint                 column,
                                         AttributeDesc::Look  look)
{
  if (_type == G_TYPE_STRING)
  {
    if (GetGType (look) == G_TYPE_STRING)
    {
      if (look == SHORT_TEXT)
      {
        gtk_tree_store_set (store, iter,
                            column, "",
                            -1);
      }
      else
      {
        gtk_tree_store_set (store, iter,
                            column, "",
                            -1);
      }
    }
    else
    {
      gtk_tree_store_set (store, iter,
                          column, NULL,
                          -1);
    }
  }
  else if (_type == G_TYPE_BOOLEAN)
  {
    gtk_tree_store_set (store, iter,
                        column, FALSE,
                        -1);
  }
  else
  {
    gtk_tree_store_set (store, iter,
                        column, 0,
                        -1);
  }
}

// --------------------------------------------------------------------------------
GType AttributeDesc::GetGType (Look look)
{
  if (   (_type == G_TYPE_STRING)
      && (look == GRAPHICAL)
      && HasDiscreteValue ())
  {
    return G_TYPE_OBJECT;
  }
  return _type;
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
gchar *AttributeDesc::GetDiscreteXmlImage (const gchar *from_user_image)
{
  if (_discrete_model)
  {
    GtkTreeIter iter;

    if (_discrete_long_text_index->GetIterFromKey (from_user_image,
                                                   &iter))
    {
      gchar *xml_image;

      gtk_tree_model_get (_discrete_model, &iter,
                          DISCRETE_XML_IMAGE_str, (void *) &xml_image,
                          -1);
      return xml_image;
    }
  }

  return g_strdup (from_user_image);
}

// --------------------------------------------------------------------------------
gchar *AttributeDesc::GetDiscreteUserImage (guint from_code)
{
  return (gchar *) GetDiscreteData (from_code,
                                    DISCRETE_LONG_TEXT_str);
}

// --------------------------------------------------------------------------------
GtkTreeIter *AttributeDesc::GetDiscreteIter (const gchar *from_user_image)
{
  if (_discrete_model)
  {
    GtkTreeIter iter;

    if (_discrete_long_text_index->GetIterFromKey (from_user_image,
                                                   &iter))
    {
      return gtk_tree_iter_copy (&iter);
    }
  }

  return NULL;
}

// --------------------------------------------------------------------------------
GdkPixbuf *AttributeDesc::GetDiscretePixbuf (guint from_code)
{
  return (GdkPixbuf *) GetDiscreteData (from_code,
                                        DISCRETE_ICON_pix);
}

// --------------------------------------------------------------------------------
GdkPixbuf *AttributeDesc::GetDiscretePixbuf (const gchar *from_value)
{
  return (GdkPixbuf *) GetDiscreteData (from_value,
                                        DISCRETE_SHORT_TEXT_str,
                                        DISCRETE_ICON_pix);
}

// --------------------------------------------------------------------------------
void *AttributeDesc::GetDiscreteData (guint from_code,
                                      guint column)
{
  if (_discrete_model)
  {
    GtkTreeIter iter;

    if (_discrete_code_index->GetIterFromKey (GUINT_TO_POINTER (from_code),
                                              &iter))
    {
      void *data;

      gtk_tree_model_get (_discrete_model, &iter,
                          column,          &data,
                          -1);
      return data;
    }
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void *AttributeDesc::GetDiscreteData (const gchar *from_user_image,
                                      guint        image_type,
                                      guint        column)
{
  if (_discrete_model)
  {
    GtkTreeIter iter;
    gboolean    iter_is_valid = FALSE;

    if (image_type == DISCRETE_SHORT_TEXT_str)
    {
      iter_is_valid = _discrete_short_text_index->GetIterFromKey (from_user_image,
                                                                  &iter);
    }
    else if (image_type == DISCRETE_LONG_TEXT_str)
    {
      iter_is_valid = _discrete_long_text_index->GetIterFromKey (from_user_image,
                                                                 &iter);
    }
    else if (image_type == DISCRETE_XML_IMAGE_str)
    {
      iter_is_valid = _discrete_xml_index->GetIterFromKey (from_user_image,
                                                           &iter);
    }

    if (iter_is_valid)
    {
      void *data;

      gtk_tree_model_get (_discrete_model, &iter,
                          column,          &data,
                          -1);
      return data;
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

    if (_discrete_long_text_index->GetIterFromKey (user_image,
                                                   &iter))
    {
      gchar *current_xml_image;

      gtk_tree_model_get (_discrete_model, &iter,
                          DISCRETE_XML_IMAGE_str, &current_xml_image,
                          -1);
      return current_xml_image;
    }
  }

  return NULL;
}

// --------------------------------------------------------------------------------
gchar *AttributeDesc::GetUserImage (GtkTreeIter *iter,
                                    Look         look)
{
  gint column;

  if (look == SHORT_TEXT)
  {
    column = DISCRETE_SHORT_TEXT_str;
  }
  else if (look == LONG_TEXT)
  {
    column = DISCRETE_LONG_TEXT_str;
  }
  else
  {
    return NULL;
  }

  if (_discrete_model)
  {
    gchar *image;

    gtk_tree_model_get (_discrete_model, iter,
                        column, &image,
                        -1);
    if (image)
    {
      return image;
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
                        DISCRETE_XML_IMAGE_str, &image, -1);
    if (image)
    {
      return image;
    }
  }

  return NULL;
}

// --------------------------------------------------------------------------------
gchar *AttributeDesc::GetUserImage (gchar *xml_image,
                                    Look   look)
{
  if (xml_image)
  {
    if (_discrete_model)
    {
      GtkTreeIter iter;

      if (_discrete_xml_index->GetIterFromKey (xml_image,
                                               &iter))
      {
        gint   column;
        gchar *current_user_image;

        if (look == SHORT_TEXT)
        {
          column = DISCRETE_SHORT_TEXT_str;
        }
        else if (look == LONG_TEXT)
        {
          column = DISCRETE_LONG_TEXT_str;
        }
        else
        {
          return NULL;
        }

        gtk_tree_model_get (_discrete_model, &iter,
                            column, &current_user_image,
                            -1);
        return current_user_image;
      }
    }
    return GetUndivadableText (xml_image);
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void AttributeDesc::AddDiscreteValues (const gchar *first_xml_image,
                                       const gchar *first_user_image,
                                       const gchar *first_icon,
                                       ...)
{
  if (_discrete_model == NULL)
  {
    CreateDiscreteModel ();
  }

  {
    va_list      ap;
    const gchar *xml_image  = first_xml_image;
    const gchar *user_image = first_user_image;
    const gchar *icon       = first_icon;

    va_start (ap, first_icon);
    while (xml_image)
    {
      GtkTreeIter iter;

      gtk_tree_store_append (GTK_TREE_STORE (_discrete_model), &iter, NULL);

      {
        gchar *undivadable_image = GetUndivadableText (user_image);

        gtk_tree_store_set (GTK_TREE_STORE (_discrete_model), &iter,
                            DISCRETE_CODE_uint,      xml_image[0],
                            DISCRETE_XML_IMAGE_str,  xml_image,
                            DISCRETE_LONG_TEXT_str,  undivadable_image,
                            DISCRETE_SHORT_TEXT_str, xml_image, -1);

        _discrete_code_index->SetIterToKey (&iter,
                                            (const void *) xml_image[0]);
        _discrete_xml_index->SetIterToKey (&iter,
                                           xml_image);
        _discrete_long_text_index->SetIterToKey (&iter,
                                                 undivadable_image);
        _discrete_short_text_index->SetIterToKey (&iter,
                                                  xml_image);

        g_free (undivadable_image);
      }

      if (icon)
      {
        GdkPixbuf *pixbuf;
        gchar     *icon_path = g_build_filename (Global::_share_dir, icon, NULL);

        if (g_file_test (icon_path, G_FILE_TEST_IS_REGULAR))
        {
          pixbuf = gdk_pixbuf_new_from_file (icon_path, NULL);
        }
        else
        {
          GtkWidget *image = gtk_image_new ();

          g_object_ref_sink (image);
          pixbuf = gtk_widget_render_icon (image,
                                           icon,
                                           GTK_ICON_SIZE_BUTTON,
                                           NULL);
          g_object_unref (image);
        }

        gtk_tree_store_set (GTK_TREE_STORE (_discrete_model), &iter,
                            DISCRETE_ICON_pix, pixbuf, -1);
        if (pixbuf)
        {
          g_object_unref (pixbuf);
        }
        g_free (icon_path);
      }

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
void AttributeDesc::CreateDiscreteModel ()
{
  _discrete_model = GTK_TREE_MODEL (gtk_tree_store_new (NB_DISCRETE_COLUMNS,
                                                        G_TYPE_UINT,     // DISCRETE_CODE_uint
                                                        G_TYPE_STRING,   // DISCRETE_XML_IMAGE_str
                                                        G_TYPE_STRING,   // DISCRETE_LONG_TEXT_str
                                                        G_TYPE_STRING,   // DISCRETE_SHORT_TEXT_str
                                                        GDK_TYPE_PIXBUF, // DISCRETE_ICON_pix
                                                        G_TYPE_STRING)); // DISCRETE_SELECTOR_str

  _discrete_code_index       = new TreeModelIndex (_discrete_model, g_direct_hash);
  _discrete_xml_index        = new TreeModelIndex (_discrete_model, g_str_hash);
  _discrete_long_text_index  = new TreeModelIndex (_discrete_model, g_str_hash);
  _discrete_short_text_index = new TreeModelIndex (_discrete_model, g_str_hash);
}

// --------------------------------------------------------------------------------
void AttributeDesc::AddDiscreteValueSelector (const gchar *name)
{
  gchar *path = g_build_filename (Global::_share_dir, "resources", name, NULL);

  CreateDiscreteModel ();

  _is_selector = TRUE;

  AddDiscreteValues (path,
                     "");

  g_free (path);
}

// --------------------------------------------------------------------------------
void AttributeDesc::AddLocalizedDiscreteValues (const gchar *name)
{
  gchar       *dir_path = g_build_filename (Global::_share_dir, "resources", "localized_data", NULL);
  GDir        *dir      = g_dir_open       (dir_path, 0,  NULL);
  const gchar *country  = g_dir_read_name  (dir);

  CreateDiscreteModel ();
  _has_selector = TRUE;

  while (country)
  {
    gchar *full_path = g_build_filename (dir_path, country, name, NULL);

    AddDiscreteValues (full_path,
                       country);

    g_free (full_path);
    country = g_dir_read_name (dir);
  }

  g_dir_close (dir);
  g_free (dir_path);
}

// --------------------------------------------------------------------------------
void AttributeDesc::AddDiscreteValues (const gchar *dir,
                                       const gchar *selector)
{
  gchar  *raw_file;
  gchar  *index_file = g_build_filename (dir, "index.txt", NULL);
  GError *error      = NULL;


  if (g_file_get_contents ((const gchar *) index_file,
                           &raw_file,
                           NULL,
                           &error) == FALSE)
  {
    if (g_error_matches (error,
                         G_FILE_ERROR,
                         G_FILE_ERROR_NOENT) == FALSE)
    {
      g_print ("AttributeDesc::AddDiscreteValues -> %s\n", error->message);
    }
    g_error_free (error);
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
          gchar *translations = g_build_filename (dir, "translations", NULL);

          if (g_file_test (translations, G_FILE_TEST_IS_DIR))
          {
            textdomain = g_path_get_basename (dir);
          }
          g_free (translations);
        }

        for (guint i = 0; (tokens[i] != NULL) && (*tokens[i] != 0); i+=nb_tokens)
        {
          GtkTreeIter iter;

          gtk_tree_store_append (GTK_TREE_STORE (_discrete_model), &iter, NULL);

          // Selector
          {
            gtk_tree_store_set (GTK_TREE_STORE (_discrete_model), &iter,
                                DISCRETE_SELECTOR_str, selector,
                                -1);
          }

          {
            gchar *undivadable_text;

            // XML code
            {
              undivadable_text = GetUndivadableText (tokens[i]);
              gtk_tree_store_set (GTK_TREE_STORE (_discrete_model), &iter,
                                  DISCRETE_XML_IMAGE_str, undivadable_text,
                                  -1);
              _discrete_xml_index->SetIterToKey (&iter,
                                                 undivadable_text);

              // Icon
              {
                gchar *escape_file     = g_uri_escape_string (tokens[i], NULL, FALSE);
                gchar *pixbuf_file     = g_build_filename (dir, escape_file, NULL);
                gchar *pixbuf_file_png = g_strdup_printf ("%s.png", pixbuf_file);

                if (g_file_test (pixbuf_file_png, G_FILE_TEST_IS_REGULAR))
                {
                  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (pixbuf_file_png, NULL);

                  gtk_tree_store_set (GTK_TREE_STORE (_discrete_model), &iter,
                                      DISCRETE_ICON_pix, pixbuf, -1);
                }
                g_free (pixbuf_file_png);
                g_free (pixbuf_file);
                g_free (escape_file);
              }
            }

            // Short name
            {
              if (*tokens[i+1] != 0)
              {
                g_free (undivadable_text);
                undivadable_text = GetUndivadableText (tokens[i+1]);
              }
              gtk_tree_store_set (GTK_TREE_STORE (_discrete_model), &iter,
                                  DISCRETE_SHORT_TEXT_str, undivadable_text,
                                  -1);
              _discrete_short_text_index->SetIterToKey (&iter,
                                                        undivadable_text);
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
                                  DISCRETE_LONG_TEXT_str, undivadable_text,
                                  -1);
              _discrete_long_text_index->SetIterToKey (&iter,
                                                       undivadable_text);
            }

            g_free (undivadable_text);
          }

          // Internal code (French federation)
          if (tokens[i+3][0])
          {
            gchar **subtokens = g_strsplit_set (tokens[i+3],
                                                 "-",
                                                 0);

            if (subtokens)
            {
              for (guint s = 0; subtokens[s] != NULL; s++)
              {
                gtk_tree_store_set (GTK_TREE_STORE (_discrete_model), &iter,
                                    DISCRETE_CODE_uint, atoi (subtokens[s]),
                                    -1);
                _discrete_code_index->SetIterToKey (&iter,
                                                    GUINT_TO_POINTER (atoi (subtokens[s])));
              }
              g_strfreev (subtokens);
            }
          }
        }
        g_strfreev (tokens);
        g_free (textdomain);
      }
    }
    g_free (raw_file);
  }

  g_free (index_file);
}

// --------------------------------------------------------------------------------
void AttributeDesc::CreateExcludingList (GSList **list, ...)
{
  GSList *new_list = NULL;
  GList  *current  = _list;

  while (current)
  {
    va_list        ap;
    gchar         *name;
    AttributeDesc *desc = (AttributeDesc *) current->data;

    va_start (ap, list);
    while ((name = va_arg (ap, char *)))
    {
      if (g_strcmp0 (name, desc->_code_name) == 0)
      {
        break;
      }
    }
    va_end (ap);

    if (name == NULL)
    {
      new_list = g_slist_append (new_list,
                                 desc);
    }

    current = g_list_next (current);
  }

  *list = new_list;
}

// --------------------------------------------------------------------------------
void AttributeDesc::CreateIncludingList (GSList **list, ...)
{
  GSList *new_list = NULL;
  GList  *current  = _list;

  while (current)
  {
    va_list        ap;
    gchar         *name;
    AttributeDesc *desc = (AttributeDesc *) current->data;

    va_start (ap, list);
    while ((name = va_arg (ap, char *)))
    {
      if (g_strcmp0 (name, desc->_code_name) == 0)
      {
        new_list = g_slist_append (new_list,
                                   desc);
        break;
      }
    }
    va_end (ap);

    current = g_list_next (current);
  }

  *list = new_list;
}

// --------------------------------------------------------------------------------
GList *AttributeDesc::GetList ()
{
  return _list;
}

// --------------------------------------------------------------------------------
AttributeDesc *AttributeDesc::GetDescFromCodeName (const gchar *code_name)
{
  GList *current = _list;

  while (current)
  {
    AttributeDesc *attr_desc = (AttributeDesc *) current->data;

    if (g_strcmp0 (attr_desc->_code_name, code_name) == 0)
    {
      return attr_desc;
    }
    current = g_list_next (current);
  }

  return NULL;
}

// --------------------------------------------------------------------------------
AttributeDesc *AttributeDesc::GetDescFromXmlName (const gchar *xml_name)
{
  GList *current = _list;

  while (current)
  {
    AttributeDesc *attr_desc = (AttributeDesc *) current->data;

    if (g_strcmp0 (attr_desc->_xml_name, xml_name) == 0)
    {
      return attr_desc;
    }
    current = g_list_next (current);
  }

  return NULL;
}

// --------------------------------------------------------------------------------
AttributeDesc *AttributeDesc::GuessDescFromUserName (const gchar *user_name,
                                                     const gchar *criteria)
{
  GList *current = _list;

  while (current)
  {
    AttributeDesc *attr_desc = (AttributeDesc *) current->data;

    if (attr_desc->MatchCriteria (criteria))
    {
      if (g_ascii_strcasecmp (user_name, attr_desc->_user_name) == 0)
      {
        return attr_desc;
      }
      else if (g_ascii_strcasecmp (user_name, attr_desc->_code_name) == 0)
      {
        return attr_desc;
      }
      else if (g_ascii_strcasecmp (user_name, attr_desc->_xml_name) == 0)
      {
        return attr_desc;
      }
    }

    current = g_list_next (current);
  }

  return NULL;
}
