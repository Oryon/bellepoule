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

class Player;
class Module;

class Attendees : public Object
{
  public:
    struct Listener
    {
      virtual void OnAttendeeToggled (Player *attendee) = 0;
    };

  public:
    Attendees (Listener *listener,
               Module   *owner);

    void SetPresents (GSList *presents);

    void SetAbsents (GSList *absents);

    GSList *GetPresents ();

    GSList *GetAbsents ();

    Module *GetOwner ();

    void Toggle (Player *fencer);

  private:
    Module   *_owner    = nullptr;
    Listener *_listener = nullptr;
    GSList   *_presents = nullptr;
    GSList   *_absents  = nullptr;

    virtual ~Attendees ();
};
