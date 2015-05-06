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

#ifndef batch_hpp
#define batch_hpp

#include <libxml/xpath.h>

#include "util/module.hpp"

class Batch : public Module
{
  public:
    Batch (const gchar *id);

    void AttachTo (GtkNotebook *to);

    void LoadJob (xmlNode   *xml_node,
                  GChecksum *sha1);

    guint GetId ();

    void SetProperties (GKeyFile *key_file);

  private:
    guint32       _id;
    GtkListStore *_list_store;
    guint32       _dnd_key;

    virtual ~Batch ();

    void SetProperty (GKeyFile    *key_file,
                      const gchar *property);

    gboolean HasJob (GChecksum *sha1);

    void OnDragDataGet (GtkWidget        *widget,
                        GdkDragContext   *drag_context,
                        GtkSelectionData *data,
                        guint             key,
                        guint             time);
};

#endif
