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

#include "light.hpp"
#include "scoring_machine.hpp"

// --------------------------------------------------------------------------------
ScoringMachine::ScoringMachine (const gchar *class_name)
  : Object (class_name)
{
}

// --------------------------------------------------------------------------------
ScoringMachine::~ScoringMachine ()
{
}

// --------------------------------------------------------------------------------
void ScoringMachine::ConnectToLights (GData *lights)
{
  g_datalist_foreach (&lights,
                      (GDataForeachFunc) ScoringMachine::ConnectToLight,
                      this);
}

// --------------------------------------------------------------------------------
void ScoringMachine::ConnectToLight (GQuark          quark,
                                     Light          *light,
                                     ScoringMachine *machine)
{
  machine->Wire (quark,
                 light);
}
