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

#include <json-glib/json-glib.h>

#include "util/object.hpp"

namespace Net
{
  class UsbDrive;

  class UsbBroker : public Object
  {
    public:
      enum class Event
      {
        STICK_PLUGGED,
        STICK_UNPLUGGED
      };

      struct Listener
      {
        virtual void OnUsbEvent (Event     event,
                                 UsbDrive *drive) = 0;
      };

    public:
      UsbBroker (Listener *listener);

    private:
      GString    *_json;
      GIOChannel *_stop_channel;
      GIOChannel *_in_channel;
      Listener   *_listener;
      GList      *_sources;
      GPid        _child_pid;
      GData      *_drives;

      ~UsbBroker () override;

      void Start ();

      void Reset ();

      void OnEvent ();

      gchar *GetField (JsonParser  *parser,
                       const gchar *field_name);

      GIOChannel *GetIoChannel (gint         fd,
                                GIOCondition cond,
                                GIOFunc      func);

      static gboolean OnUsbBackendData (GIOChannel   *channel,
                                        GIOCondition  cond,
                                        UsbBroker   *broker);
  };
}
