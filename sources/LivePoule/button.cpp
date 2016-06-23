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

#include "button.hpp"

// --------------------------------------------------------------------------------
Button::Button (guint       pin_id,
                GSourceFunc handler,
                void        *context)
  : Gpio (pin_id,
          (GSourceFunc) OnButtonEvent,
          this)
{
  _debounced_handler = handler;
  _debounced_context = context;

  _debounce_started = FALSE;
}

// --------------------------------------------------------------------------------
Button::~Button ()
{
}

// --------------------------------------------------------------------------------
gboolean Button::OnDebounceStop (Button *button)
{
  button->_debounce_started = FALSE;
  return button->_debounced_handler (button->_debounced_context);
}

// --------------------------------------------------------------------------------
gboolean Button::OnButtonEvent (Button *button)
{
  if (button->_debounce_started == FALSE)
  {
    button->_debounce_started = TRUE;
    g_timeout_add (50,
                   (GSourceFunc) OnDebounceStop,
                   button);
  }

  return G_SOURCE_REMOVE;
}
