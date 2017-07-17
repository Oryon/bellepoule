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

#pragma once

#include "util/object.hpp"

class Category : public Object
{
  public:
    Category (GtkWidget *combo_box);

    void Replicate (Category *what);

    void ParseXml (const gchar *xml);

    void ReadUserChoice ();

    const gchar *GetDisplayImage ();

    const gchar *GetXmlImage ();

  private:
    static const guint _nb_category = 16;
    static const gchar *_display_image[_nb_category];
    static const gchar *_xml_image[_nb_category];
    static const gchar *_xml_alias[_nb_category];

    GtkComboBox *_combo_box;
    GtkEntry    *_entry;
    gint         _id;
    gchar       *_fallback;

    virtual ~Category ();

    void RefreshComboBox ();
};
