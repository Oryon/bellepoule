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

class Data;

class Score : public Object
{
  public:
    Score (Data *max);

  public:
    void Set (guint score, gboolean victory);

    void Drop (gchar *reason);

    void Restore ();

    void Clean ();

    void SynchronizeWith (Score *with);

  public:
    guint Get ();

    gchar *GetImage ();

    gchar GetDropReason ();

    const gchar *GetStatusImage ();

  public:
    gboolean IsKnown ();

    gboolean IsOut ();

    gboolean IsValid ();

    gboolean IsTheBest ();

    gboolean IsConsistentWith (Score *with);

  private:
    typedef enum
    {
      UNKNOWN,
      VICTORY,
      DEFEAT,
      WITHDRAWAL,
      BLACK_CARD,
      OPPONENT_OUT
    } Status;

    Data   *_max;
    guint   _score;
    Status  _status;
    Status  _backup_status;

    virtual ~Score ();
};
