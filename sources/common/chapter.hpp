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

#ifndef chapter_hpp
#define chapter_hpp

#include "object.hpp"
#include "module.hpp"

class Chapter : public Object
{
  public:
    Chapter (Module *module,
             guint   first_page,
             guint   last_page);

    Module *GetModule ();

    guint GetFirstPage ();

    guint GetLastPage ();

  private:
    Module *_module;
    guint   _first_page;
    guint   _last_page;

    virtual ~Chapter ();
};

#endif
