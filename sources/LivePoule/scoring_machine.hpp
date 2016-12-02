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
#include "light.hpp"

class ScoringMachine : public Object
{
  public:
    void ConnectToLights (GData *lights);

  protected:
    ScoringMachine (const gchar *class_name);

    virtual ~ScoringMachine ();

    virtual void Wire (GQuark  quark,
                       Light  *light) = 0;

  private:
    static void ConnectToLight (GQuark          quark,
                                Light          *light,
                                ScoringMachine *machine);
};
