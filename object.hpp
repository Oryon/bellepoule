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

#ifndef object_hpp
#define object_hpp

#include <gtk/gtk.h>

class Object_c
{
  public:
    void SetData (Object_c       *owner,
                  gchar          *key,
                  void           *data,
                  GDestroyNotify  destroy_cbk = NULL);

    void *GetData (Object_c *owner,
                   gchar    *key);

    void Retain ();

    void Release ();

    static void Dump ();

    static void Release (Object_c *object);

  protected:
    virtual ~Object_c ();

    Object_c (gchar *class_name = NULL);

  private:
    GData *_datalist;
    guint  _ref_count;
    gchar *_class_name;

    static GList *_list;
    static guint  _nb_objects;
};

#endif
