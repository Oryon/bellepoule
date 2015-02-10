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

#ifndef language_hpp
#define language_hpp

#include "util/object.hpp"

class Language : public Object
{
  public:
    Language ();

    void Populate (GtkMenuItem  *menu_item,
                   GtkMenuShell *menu_shell);

  private:
    virtual ~Language ();

    static void OnLocaleToggled (GtkCheckMenuItem *checkmenuitem,
                                 gchar            *locale);
};

#endif
