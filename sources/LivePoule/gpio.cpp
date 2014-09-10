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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#ifdef WIRING_PI
#include <wiringPi.h>
#endif

#include "gpio.hpp"

// --------------------------------------------------------------------------------
Gpio::Gpio (guint        pin,
            EventHandler handler)
  : Object ("Gpio")
{
  _pin           = pin;
  _event_handler = handler;

#ifdef WIRING_PI
  pinMode         (_pin, INPUT);
  pullUpDnControl (_pin, PUD_UP);

  if (wiringPiISR (pin,
                   INT_EDGE_BOTH,
                   handler) < 0)
  {
    g_warning ("OnSignal[%d] registration error", pin);
  }
#endif
}

// --------------------------------------------------------------------------------
Gpio::~Gpio ()
{
}

// --------------------------------------------------------------------------------
void Gpio::Init ()
{
#ifdef WIRING_PI
  wiringPiSetup () ;
#endif
}

// --------------------------------------------------------------------------------
guint Gpio::GetState ()
{
#ifdef WIRING_PI
  return digitalRead (_pin);
#else
  return 0;
#endif
}
