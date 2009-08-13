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

#ifndef stage_hpp
#define stage_hpp

#include <libxml/xmlwriter.h>
#include <gtk/gtk.h>

#include "object.hpp"
#include "module.hpp"

class PlayersBase_c;

class Stage_c : public Object_c
{
  public:
    Stage_c (Module_c *owner,
             Module_c *module,
             gchar    *name);

    void Save (xmlTextWriter *xml_writer);
    void Load (xmlNode       *xml_node);

    gchar *GetName ();

    void Plug (GtkWidget *in);

    void Lock ();

  protected:
    virtual ~Stage_c ();

  private:
    gchar    *_name;
    Module_c *_module;
    Module_c *_owner;
};

#endif
