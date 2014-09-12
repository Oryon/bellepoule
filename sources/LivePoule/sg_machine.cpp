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

#include "sg_machine.hpp"

// --------------------------------------------------------------------------------
SgMachine::SgMachine ()
  : ScoringMachine ("SgMachine")
{
}

// --------------------------------------------------------------------------------
SgMachine::~SgMachine ()
{
}

// --------------------------------------------------------------------------------
void SgMachine::Wire (GQuark  quark,
                      Light  *light)
{
  if (quark == g_quark_from_string ("red_hit_light"))
  {
    light->WireGpioPin ("non-valid", 0);
    light->WireGpioPin ("valid",     1);
  }
  else if (quark == g_quark_from_string ("red_failure_light"))
  {
    light->WireGpioPin ("on", 2);
  }
  else if (quark == g_quark_from_string ("green_hit_light"))
  {
    light->WireGpioPin ("non-valid", 3);
    light->WireGpioPin ("valid",     4);
  }
  else if (quark == g_quark_from_string ("green_failure_light"))
  {
    light->WireGpioPin ("on", 5);
  }
}
