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
ScoringMachine::ScoringMachine ()
  : Object ("ScoringMachine")
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
                      (GDataForeachFunc) ScoringMachine::OnConnect,
                      this);
}

// --------------------------------------------------------------------------------
void ScoringMachine::OnConnect (GQuark          quark,
                                Light          *light,
                                ScoringMachine *machine)
{
  if (quark == g_quark_from_string ("red_hit_light"))
  {
    light->WireGpioPin ("valid",     0);
    light->WireGpioPin ("non-valid", 1);
  }
  else if (quark == g_quark_from_string ("red_failure_light"))
  {
    light->WireGpioPin ("on", 2);
  }
  else if (quark == g_quark_from_string ("green_hit_light"))
  {
    light->WireGpioPin ("valid",     3);
    light->WireGpioPin ("non-valid", 4);
  }
  else if (quark == g_quark_from_string ("green_failure_light"))
  {
    light->WireGpioPin ("on", 5);
  }
}
