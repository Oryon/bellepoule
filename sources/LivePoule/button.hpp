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

#ifndef button_hpp
#define button_hpp

#include "gpio.hpp"

class Button : public Gpio
{
  public:
    Button (guint        pin_id,
            GSourceFunc  handler,
            void        *context);

  private:
    GSourceFunc  _debounced_handler;
    void        *_debounced_context;
    guint        _debounce_started;

    virtual ~Button ();

    static gboolean OnButtonEvent (Button *button);

    static gboolean OnDebounceStop (Button *button);

    static gpointer MainLoop (Button *button);
};

#endif
