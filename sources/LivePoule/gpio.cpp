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

#include <wiringPi.h>

#include "gpio.hpp"

// --------------------------------------------------------------------------------
Gpio::Gpio (guint        pin_id,
            GSourceFunc  handler,
            void        *context)
  : Object ("Gpio")
{
  _pin_id        = wpiPinToGpio (pin_id);
  _event_handler = handler;
  _context       = context;

  SetPinMode ("in"); // pinMode         (_pin_id, INPUT);
  SetPinMode ("up"); // pullUpDnControl (_pin_id, PUD_UP);

  if (wiringPiISR (_pin_id,
                   INT_EDGE_BOTH,
                   (void (*) (void *)) OnEvent,
                   this) < 0)
  {
    g_warning ("OnSignal[%d] registration error", _pin_id);
  }
}

// --------------------------------------------------------------------------------
Gpio::~Gpio ()
{
}

// --------------------------------------------------------------------------------
void Gpio::Init ()
{
  wiringPiSetupSys () ;
}

// --------------------------------------------------------------------------------
guint Gpio::GetVoltageState ()
{
  return digitalRead (_pin_id);
}

// --------------------------------------------------------------------------------
void Gpio::OnEvent (Gpio *gpio)
{
  g_idle_add ((GSourceFunc) gpio->_event_handler,
              gpio->_context);
}

// --------------------------------------------------------------------------------
void Gpio::SetPinMode (const gchar *mode)
{
  GError *error           = NULL;
  gchar  *standard_output;
  gchar  *standard_error;
  gint    exit_status;
  gchar  *cmd;

  cmd = g_strdup_printf ("gpio -g mode %d %s", _pin_id, mode);
  g_spawn_command_line_sync (cmd,
                             &standard_output,
                             &standard_error,
                             &exit_status,
                             &error);
  if (error)
  {
    g_warning ("g_spawn_command_line_sync: %s", error->message);
    g_clear_error (&error);
  }
  g_free (cmd);
}
