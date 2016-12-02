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

    guint GetVoltageState ();

  protected:
    virtual ~Gpio ();

  private:
    guint         _pin_id;
    GSourceFunc   _event_handler;
    void         *_context;

    static void OnEvent (Gpio *gpio);

    void SetPinMode (const gchar *mode);
};
