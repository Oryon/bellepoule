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

#ifndef referee_zone_hpp
#define referee_zone_hpp

#include <gtk/gtk.h>

#include "drop_zone.hpp"

class Player;

class RefereeZone : public DropZone
{
  public:
    RefereeZone (Module *container);

    void AddObject (Object *object);

    void RemoveObject (Object *object);

    virtual void BookReferees ();

    virtual void FreeReferees ();

    void AllowBooking ();

    void ForbidBooking ();

  protected:
    GSList  *_referee_list;

    virtual ~RefereeZone ();

    virtual void AddReferee (Player *referee);

    virtual void RemoveReferee (Player *referee);

    virtual void BookReferee (Player *referee);

    void FreeReferee (Player *referee);

  private:
    gboolean _manage_booking;

    virtual guint GetNbMatchs ();
};

#endif
