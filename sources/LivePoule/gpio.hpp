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

class Gpio : public Object
{
  public:
    static void Init ();

    Gpio (guint        pin_id,
          GSourceFunc  handler,
          void        *context);

    void GenerateFakeEvent ();

    guint GetVoltageState ();

  protected:
    virtual ~Gpio ();

  private:
    typedef void (*Callback)();
    struct Pin
    {
      Gpio     *_gpio;
      Callback  _callback;
    };

    static const guint WIRED_PINS = 9;
    static Pin _wired_pins[WIRED_PINS];

  private:
    guint        _bcm_id;
    guint        _fake_voltage;
    GSourceFunc  _event_handler;
    void        *_context;

    void OnEvent ();

    void SetPinMode (const gchar *mode);

    static gpointer FakeLoop (Gpio *gpio);

  private:
    // What a dirty workaround!
    // Usually callbacks have a user-specified parameter acting
    // as a "context". This is obviously a very valuable information,
    // but unfortunatelay WiringPi doesn't offer it :(
    // No other choice that wiring a dedicated callback for each pin!
    static void OnPin0 ();
    static void OnPin1 ();
    static void OnPin2 ();
    static void OnPin3 ();
    static void OnPin4 ();
    static void OnPin5 ();
    static void OnPin6 ();
    static void OnPin7 ();
    static void OnPin8 ();
};
