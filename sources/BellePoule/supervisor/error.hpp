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

#include <gtk/gtk.h>

#include "util/object.hpp"

class Error : public Object
{
  public:
    typedef enum
    {
      LEVEL_ERROR,
      LEVEL_WARNING
    } Level;

    class Provider
    {
      public:
        virtual Error *SpawnError () = 0;

      protected:
        Provider () {};
    };

  public:
    Error (Level        level,
           const gchar *guilty_party,
           const gchar *reason);

    gchar *GetMessage ();

    const gchar *GetColor ();

  private:
    Level  _level;
    gchar *_guilty_party;
    gchar *_reason;

    virtual ~Error ();
};
