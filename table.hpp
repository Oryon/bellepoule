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

#ifndef table_hpp
#define table_hpp

#include <gtk/gtk.h>

#include "stage.hpp"
#include "canvas_module.hpp"

class Table : public virtual Stage_c, public CanvasModule_c
{
  public:
    static void Init ();

    Table (StageClass *stage_class);

    void Save (xmlTextWriter *xml_writer);

  public:
    static const gchar *_class_name;
    static const gchar *_xml_class_name;

  private:
    void Enter ();
    void OnLocked ();
    void OnUnLocked ();
    void Wipe ();

  private:
    void Display ();
    void OnPlugged ();
    void OnAttrListUpdated ();

    static Stage_c *CreateInstance (StageClass *stage_class);

    void Load (xmlNode *xml_node);

    ~Table ();
};

#endif
