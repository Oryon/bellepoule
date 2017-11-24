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

#include "category.hpp"

const gchar *Category::_display_image[Category::_nb_category] =
{
  N_ ("U5"),
  N_ ("U7"),
  N_ ("U9"),
  N_ ("U11"),
  N_ ("U13"),
  N_ ("U15"),
  N_ ("U17"),
  N_ ("U20"),
  N_ ("U23"),
  N_ ("Senior"),
  N_ ("Veteran"),
  N_ ("Veteran 1"),
  N_ ("Veteran 2"),
  N_ ("Veteran 3"),
  N_ ("Veteran 4"),
  N_ ("Veteran 3+4")
};

const gchar *Category::_xml_image[_nb_category] =
{
  "M5",
  "M7",
  "M9",
  "M11",
  "M13",
  "M15",
  "M17",
  "M20",
  "M23",
  "S",
  "VET",
  "V1",
  "V2",
  "V3",
  "V4",
  "V34"
};

const gchar *Category::_xml_alias[_nb_category] =
{
  "M5",
  "M7",
  "PO",
  "PUP",
  "B",
  "M",
  "C",
  "J",
  "M23",
  "S",
  "VET",
  "V1",
  "V2",
  "V3",
  "V4",
  "V34"
};

typedef enum
{
  COMBO_CATEGORY_DISPLAY_NAME_str,

  NB_COMBO_CATEGORY_COLUMNS
} ComboCategoryColumnId;

// --------------------------------------------------------------------------------
Category::Category (GtkWidget *combo_box)
  : Object ("Category")
{
  _combo_box = GTK_COMBO_BOX (combo_box);
  _entry     = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (_combo_box)));
  _id        = 0;
  _fallback  = NULL;

  {
    GtkTreeModel *model = gtk_combo_box_get_model (_combo_box);

    for (guint i = 0; i < _nb_category; i ++)
    {
      GtkTreeIter iter;

      gtk_list_store_append (GTK_LIST_STORE (model),
                             &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          COMBO_CATEGORY_DISPLAY_NAME_str, gettext (_display_image[i]),
                          -1);
    }
  }

  RefreshComboBox ();
}

// --------------------------------------------------------------------------------
Category::~Category ()
{
  g_free (_fallback);
}

// --------------------------------------------------------------------------------
void Category::ReadUserChoice ()
{
  g_free (_fallback);
  _fallback = NULL;

  _id = gtk_combo_box_get_active (_combo_box);

  if (_id == -1)
  {
    _fallback = g_strdup (gtk_entry_get_text (_entry));

    for (guint i = 0; i < _nb_category; i++)
    {
      if (g_strcmp0 (_fallback, gettext (_display_image[i])) == 0)
      {
        _id = i;

        g_free (_fallback);
        _fallback = NULL;
        break;
      }
    }
  }
}

// --------------------------------------------------------------------------------
void Category::RefreshComboBox ()
{
  gtk_combo_box_set_active (_combo_box,
                            _id);

  if (_id == -1)
  {
    gtk_entry_set_text (_entry,
                        _fallback);
  }
}

// --------------------------------------------------------------------------------
void Category::Replicate (Category *what)
{
  _id = what->_id;

  g_free (_fallback);
  _fallback = g_strdup (what->_fallback);

  RefreshComboBox ();
}

// --------------------------------------------------------------------------------
const gchar *Category::GetDisplayImage ()
{
  if (_id != -1)
  {
    return gettext (_display_image[_id]);
  }

  return _fallback;
}

// --------------------------------------------------------------------------------
const gchar *Category::GetXmlImage ()
{
  if (_id != -1)
  {
    return _xml_image[_id];
  }

  return _fallback;
}

// --------------------------------------------------------------------------------
void Category::ParseXml (const gchar *xml)
{
  _id = -1;

  g_free (_fallback);
  _fallback = NULL;

  for (guint i = 0; i < _nb_category; i++)
  {
    if (g_strcmp0 (xml, _xml_image[i]) == 0)
    {
      _id = i;
      break;
    }
  }

  if (_id == -1)
  {
    for (guint i = 0; i < _nb_category; i++)
    {
      if (g_strcmp0 (xml, _xml_alias[i]) == 0)
      {
        _id = i;
        break;
      }
    }
  }

  if (_id == -1)
  {
    _fallback = g_strdup (xml);
  }

  RefreshComboBox ();
}
