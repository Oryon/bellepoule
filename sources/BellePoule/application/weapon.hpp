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

class Weapon : public Object
{
  public:
    Weapon (const gchar *image,
            const gchar *xml_image,
            const gchar *greg_image);

  public:
    static GSList *GetList ();

    static Weapon *GetDefault ();

    static Weapon *GetFromXml (const gchar *xml);

    static Weapon *GetFromIndex (guint index);

  public:
    const gchar *GetImage ();

    const gchar *GetXmlImage ();

    const gchar *GetGregImage ();

    guint GetIndex ();

  private:
    static GSList *_list;

    gchar *_image;
    gchar *_xml_image;
    gchar *_greg_image;

    virtual ~Weapon ();
};
