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

Gpio::Pin Gpio::_wired_pins[9];

// --------------------------------------------------------------------------------
Gpio::Gpio (guint        pin_id,
            GSourceFunc  handler,
            void        *context)
  : Object ("Gpio")
{
  _event_handler = handler;
  _context       = context;
  _fake_voltage  = 0;

#ifdef WIRING_PI
  if (pin_id >= WIRED_PINS)
  {
    g_warning ("Gpio pin #%d not wired", pin_id);
  }
  else
  {
    _bcm_id = wpiPinToGpio (pin_id);

    _wired_pins[pin_id]._gpio = this;

    SetPinMode ("in"); // pinMode         (_bcm_id, INPUT);
    SetPinMode ("up"); // pullUpDnControl (_bcm_id, PUD_UP);

    if (wiringPiISR (_bcm_id,
                     INT_EDGE_BOTH,
                     _wired_pins[pin_id]._callback) < 0)
    {
      g_warning ("OnSignal[%d] registration error", _bcm_id);
    }
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
  _wired_pins[0]._gpio     = NULL;
  _wired_pins[0]._callback = OnPin0;
  _wired_pins[1]._gpio     = NULL;
  _wired_pins[1]._callback = OnPin1;
  _wired_pins[2]._gpio     = NULL;
  _wired_pins[2]._callback = OnPin2;
  _wired_pins[3]._gpio     = NULL;
  _wired_pins[3]._callback = OnPin3;
  _wired_pins[4]._gpio     = NULL;
  _wired_pins[4]._callback = OnPin4;
  _wired_pins[5]._gpio     = NULL;
  _wired_pins[5]._callback = OnPin5;
  _wired_pins[6]._gpio     = NULL;
  _wired_pins[6]._callback = OnPin6;
  _wired_pins[7]._gpio     = NULL;
  _wired_pins[7]._callback = OnPin7;
  _wired_pins[8]._gpio     = NULL;
  _wired_pins[8]._callback = OnPin8;

#ifdef WIRING_PI
  wiringPiSetupSys () ;
#endif
}

// --------------------------------------------------------------------------------
void Gpio::GenerateFakeEvent ()
{
#if 0
  g_thread_new (NULL,
                (GThreadFunc) FakeLoop,
                this);
#endif
}

// --------------------------------------------------------------------------------
guint Gpio::GetVoltageState ()
{
#ifdef WIRING_PI
  return digitalRead (_bcm_id);
#else
  return _fake_voltage;
#endif
}

// --------------------------------------------------------------------------------
void Gpio::OnEvent ()
{
  g_idle_add ((GSourceFunc) _event_handler,
              _context);
}

// --------------------------------------------------------------------------------
void Gpio::SetPinMode (const gchar *mode)
{
#ifdef WIRING_PI
  GError *error           = NULL;
  gchar  *standard_output;
  gchar  *standard_error;
  gint    exit_status;
  gchar  *cmd;

  cmd = g_strdup_printf ("gpio -g mode %d %s", _bcm_id, mode);
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
#endif
}

// --------------------------------------------------------------------------------
gpointer Gpio::FakeLoop (Gpio *gpio)
{
  while (1)
  {
    gint32 delay = g_random_int_range (1, 10);

    sleep (delay);

    if (gpio->_fake_voltage)
    {
      gpio->_fake_voltage = 0;
    }
    else
    {
      gpio->_fake_voltage = 1;
    }

    gpio->OnEvent ();
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void Gpio::OnPin0 ()
{
  if (_wired_pins[0]._gpio)
  {
    _wired_pins[0]._gpio->OnEvent ();
  }
}
void Gpio::OnPin1 ()
{
  if (_wired_pins[1]._gpio)
  {
    _wired_pins[1]._gpio->OnEvent ();
  }
}
void Gpio::OnPin2 ()
{
  if (_wired_pins[2]._gpio)
  {
    _wired_pins[2]._gpio->OnEvent ();
  }
}
void Gpio::OnPin3 ()
{
  if (_wired_pins[3]._gpio)
  {
    _wired_pins[3]._gpio->OnEvent ();
  }
}
void Gpio::OnPin4 ()
{
  if (_wired_pins[4]._gpio)
  {
    _wired_pins[4]._gpio->OnEvent ();
  }
}
void Gpio::OnPin5 ()
{
  if (_wired_pins[5]._gpio)
  {
    _wired_pins[5]._gpio->OnEvent ();
  }
}
void Gpio::OnPin6 ()
{
  if (_wired_pins[6]._gpio)
  {
    _wired_pins[6]._gpio->OnEvent ();
  }
}
void Gpio::OnPin7 ()
{
  if (_wired_pins[7]._gpio)
  {
    _wired_pins[7]._gpio->OnEvent ();
  }
}
void Gpio::OnPin8 ()
{
  if (_wired_pins[8]._gpio)
  {
    _wired_pins[8]._gpio->OnEvent ();
  }
}
