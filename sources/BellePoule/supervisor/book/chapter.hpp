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

class BookSection;
class Module;

class Chapter : public Object
{
  public:
    Chapter (Module           *module,
             BookSection      *section,
             Stage::StageView  stage_view,
             guint             first_page,
             guint             page_count);

    Module *GetModule ();

    guint GetFirstPage ();

    guint GetLastPage ();

    void DrawFrontPage (GtkPrintOperation *operation,
                        GtkPrintContext   *context,
                        Module            *owner);

    void DrawPage (GtkPrintOperation *operation,
                   GtkPrintContext   *context,
                   gint               page);

  private:
    Module           *_module;
    BookSection      *_section;
    gchar            *_name;
    guint             _first_page;
    guint             _last_page;
    Stage::StageView  _stage_view;

    virtual ~Chapter ();

    void ConfigurePrintOperation (GtkPrintOperation *operation);
};
