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

namespace AskFred
{
  class Club : public Object
  {
    public:
      gchar *_name;
      gchar *_division;

      Club ();

    private:
      ~Club ();
  };
}

namespace AskFred
{
  class Fencer : public Object
  {
    public:
      gchar *_last_name;
      gchar *_first_name;
      gchar *_birth;
      gchar *_gender;
      gchar *_licence;
      Club  *_club;
      gchar *_rating_saber;
      gchar *_rating_epee;
      gchar *_rating_foil;
      guint  _ranking_saber;
      guint  _ranking_epee;
      guint  _ranking_foil;

      Fencer ();

      void SetRanking (const gchar *weapon,
                       const gchar *usfa_rating);

      const gchar *GetRating (const gchar *weapon);

      guint GetRanking (const gchar *weapon);

    private:
      ~Fencer ();
  };
}

namespace AskFred
{
  class Event : public Object
  {
    public:
      const gchar *_name;
      const gchar *_location;
      const gchar *_organizer;

      gchar *_gender;
      gchar *_weapon;
      gchar *_category;

      guint _hour;
      guint _minute;
      guint _day;
      guint _month;
      guint _year;

      GList *_fencers;

      Event (const gchar *name,
             const gchar *location,
             const gchar *organizer);

    private:
      ~Event ();
  };
}

namespace AskFred
{
  class Parser : public Object
  {
    public:
      Parser (const gchar *filename);

      GList *GetEvents ();

    private:
      gchar *_name;
      gchar *_location;
      gchar *_organizer;

      GList             *_events;
      GHashTable        *_fencers;
      GHashTable        *_clubs;
      static GHashTable *_divisions;

      virtual ~Parser ();

      gboolean CheckData (xmlXPathContext *context);

      void LoadTournament (xmlXPathContext *context);

      void LoadFencers (xmlXPathContext *context);

      void LoadClubs (xmlXPathContext *context);

      void LoadEvents (xmlXPathContext *context);
  };
}
