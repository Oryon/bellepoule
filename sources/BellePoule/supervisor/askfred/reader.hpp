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
#include "util/xml_scheme.hpp"

class Player;

namespace AskFred
{
  namespace Reader
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

    class Competitor : public Object
    {
      public:
        gchar *_competitor_id;
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
        GList *_members;

        Competitor ();

        void SetRanking (const gchar *weapon,
                         const gchar *usfa_rating);

        Player *CreatePlayer (const gchar *weapon);

      private:
        ~Competitor ();

        const gchar *GetRating (const gchar *weapon);

        guint GetRanking (const gchar *weapon);
    };

    class Event : public Object
    {
      public:
        const gchar *_name;
        const gchar *_location;
        const gchar *_organizer;
        const gchar *_tournament_id;
        gchar       *_event_id;

        gchar    *_gender;
        gchar    *_weapon;
        gchar    *_category;
        gboolean  _team_event;

        guint _hour;
        guint _minute;
        guint _day;
        guint _month;
        guint _year;

        GList *_competitors;

        Event (const gchar *name,
               const gchar *location,
               const gchar *organizer,
               const gchar *tournament_id);

      private:
        ~Event ();
    };

    class Parser : public Object
    {
      public:
        Parser (const gchar *filename);

        GList *GetEvents ();

      private:
        gchar *_name;
        gchar *_location;
        gchar *_organizer;
        gchar *_tournament_id;

        GList             *_events;
        GHashTable        *_competitors;
        GHashTable        *_clubs;
        static GHashTable *_divisions;

        virtual ~Parser ();

        gboolean CheckData (xmlXPathContext *context);

        void LoadTournament (xmlXPathContext *context);

        void LoadCompetitors (xmlXPathContext *context);

        void LoadTeams (xmlXPathContext *context);

        void LoadClubs (xmlXPathContext *context);

        void LoadEvents (xmlXPathContext *context);
    };
  }
}
