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

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "ask_fred.hpp"

namespace AskFred
{
  // --------------------------------------------------------------------------------
  Club::Club ()
    : Object ("AskFred::Club")
  {
    _name = NULL;
  }

  // --------------------------------------------------------------------------------
  Club::~Club ()
  {
    g_free (_name);
  }
}

namespace AskFred
{
  // --------------------------------------------------------------------------------
  Fencer::Fencer ()
    : Object ("AskFred::Fencer")
  {
    _last_name  = NULL;
    _first_name = NULL;
    _birth      = NULL;
    _gender     = NULL;
    _licence    = NULL;
    _club       = NULL;

    _rating_saber  = NULL;
    _rating_epee   = NULL;
    _rating_foil   = NULL;
    _ranking_saber = 9999;
    _ranking_epee  = 9999;
    _ranking_foil  = 9999;
  }

  // --------------------------------------------------------------------------------
  Fencer::~Fencer ()
  {
    g_free (_last_name);
    g_free (_first_name);
    g_free (_birth);
    g_free (_gender);
    g_free (_licence);
    g_free (_rating_saber);
    g_free (_rating_epee);
    g_free (_rating_foil);
  }

  // --------------------------------------------------------------------------------
  void Fencer::SetRanking (const gchar *weapon,
                           const gchar *usfa_rating)
  {
    if (strlen (usfa_rating) == 5)
    {
      gchar letter = usfa_rating[0];

      if (g_ascii_isalpha (letter))
      {
        guint      ranking     = 1000*(1 + letter - 'A');
        GDateYear  rating_year = g_ascii_strtoll (&usfa_rating[1], NULL, 10);
        GDate     *date        = g_date_new ();

        g_date_set_time_t (date,
                           time (NULL));

        ranking += 100 * (g_date_get_year (date) - rating_year);

        if (g_strcmp0 (weapon, "Foil") == 0)
        {
          _rating_foil  = g_strdup (usfa_rating);
          _ranking_foil = ranking;
        }
        else if (g_strcmp0 (weapon, "Epee") == 0)
        {
          _rating_epee  = g_strdup (usfa_rating);
          _ranking_epee = ranking;
        }
        else if (g_strcmp0 (weapon, "Saber") == 0)
        {
          _rating_saber  = g_strdup (usfa_rating);
          _ranking_saber = ranking;
        }

        g_date_free (date);
      }
    }
  }

  // --------------------------------------------------------------------------------
  const gchar *Fencer::GetRating (const gchar *weapon)
  {
    gchar *rating = NULL;

    if (weapon)
    {
      if (weapon[0] == 'F')
      {
        rating = _rating_foil;
      }
      else if (weapon[0] == 'E')
      {
        rating = _rating_epee;
      }
      else if (weapon[0] == 'S')
      {
        rating = _rating_saber;
      }
    }

    if (rating)
    {
      return rating;
    }

    return "U";
  }

  // --------------------------------------------------------------------------------
  guint Fencer::GetRanking (const gchar *weapon)
  {
    if (weapon)
    {
      if (weapon[0] == 'F')
      {
        return _ranking_foil;
      }
      else if (weapon[0] == 'E')
      {
        return _ranking_epee;
      }
      else if (weapon[0] == 'S')
      {
        return _ranking_saber;
      }
    }

    return 9999;
  }
}

namespace AskFred
{
  // --------------------------------------------------------------------------------
  Event::Event (const gchar *name,
                const gchar *location,
                const gchar *organizer)
  : Object ("AskFred::Event")
  {
    _name      = name;
    _location  = location;
    _organizer = organizer;

    _gender   = NULL;
    _weapon   = NULL;
    _category = NULL;

    _fencers = NULL;
  }

  // --------------------------------------------------------------------------------
  Event::~Event ()
  {
    g_free (_gender);
    g_free (_weapon);
    g_free (_category);

    g_list_free (_fencers);
  }
}

namespace AskFred
{
  GHashTable *Parser::_divisions = NULL;

  // --------------------------------------------------------------------------------
  Parser::Parser (const gchar *filename)
    : Object ("AskFred::Parser")
  {
    _name       = NULL;
    _location   = NULL;
    _organizer  = NULL;

    _events = NULL;

    _fencers = g_hash_table_new_full (g_direct_hash,
                                      g_direct_equal,
                                      NULL,
                                      (GDestroyNotify) Object::TryToRelease);

    _clubs = g_hash_table_new_full (g_direct_hash,
                                    g_direct_equal,
                                    NULL,
                                    (GDestroyNotify) Object::TryToRelease);

    if (_divisions == NULL)
    {
      _divisions = g_hash_table_new_full (g_direct_hash,
                                          g_direct_equal,
                                          NULL,
                                          NULL);

      g_hash_table_insert (_divisions, GINT_TO_POINTER (1),   (gpointer) "Western WA");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (2),   (gpointer) "Inland Emp");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (3),   (gpointer) "Central CA");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (4),   (gpointer) "Mt. Valley");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (5),   (gpointer) "North Coast");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (6),   (gpointer) "Northern CA");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (7),   (gpointer) "Orange Coast");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (8),   (gpointer) "San Bern'do");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (9),   (gpointer) "San Diego");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (10),  (gpointer) "Southern CA");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (11),  (gpointer) "Oregon");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (12),  (gpointer) "Utah-S.Idaho");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (13),  (gpointer) "Alaska");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (14),  (gpointer) "Nevada");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (15),  (gpointer) "Hawaii");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (18),  (gpointer) "Columbus OH");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (19),  (gpointer) "Indiana");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (20),  (gpointer) "Michigan");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (21),  (gpointer) "Northern OH");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (22),  (gpointer) "Kentucky");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (23),  (gpointer) "Southwest OH");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (24),  (gpointer) "Metro NYC");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (25),  (gpointer) "Capitol");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (26),  (gpointer) "Central PA");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (27),  (gpointer) "Harrisburg");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (28),  (gpointer) "Maryland");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (29),  (gpointer) "New Jersey");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (30),  (gpointer) "Philadelphia");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (31),  (gpointer) "S. Jersey");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (32),  (gpointer) "Western PA");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (33),  (gpointer) "Illinois");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (34),  (gpointer) "Iowa");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (35),  (gpointer) "Minnesota");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (36),  (gpointer) "St. Louis");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (37),  (gpointer) "Wisconsin");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (39),  (gpointer) "Connecticut");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (40),  (gpointer) "Huds-Berks");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (41),  (gpointer) "Long Island");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (42),  (gpointer) "New England");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (43),  (gpointer) "Northeast");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (44),  (gpointer) "West-Rock");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (45),  (gpointer) "Western NY");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (46),  (gpointer) "Arizona");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (47),  (gpointer) "Border TX");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (48),  (gpointer) "Colorado");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (49),  (gpointer) "Kansas");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (50),  (gpointer) "Nebr-S.Dak");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (51),  (gpointer) "New Mexico");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (52),  (gpointer) "Plains TX");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (53),  (gpointer) "Wyoming");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (54),  (gpointer) "Alabama");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (55),  (gpointer) "Central FL");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (56),  (gpointer) "Gateway FL");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (57),  (gpointer) "Georgia");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (58),  (gpointer) "Gold Coast");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (59),  (gpointer) "N. Carolina");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (60),  (gpointer) "S. Carolina");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (61),  (gpointer) "Tennessee");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (62),  (gpointer) "Virginia");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (63),  (gpointer) "Ark-La-Miss");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (64),  (gpointer) "Gulf Coast");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (65),  (gpointer) "Louisiana");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (66),  (gpointer) "North TX");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (67),  (gpointer) "Oklahoma");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (68),  (gpointer) "South TX");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (69),  (gpointer) "National");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (82),  (gpointer) "Green Mt.");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (294), (gpointer) "Northeast PA");

      g_hash_table_insert (_divisions, GINT_TO_POINTER (70),  (gpointer) "British Columbia");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (71),  (gpointer) "Alberta");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (72),  (gpointer) "Saskatchewan");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (73),  (gpointer) "Manitoba");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (74),  (gpointer) "Yukon");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (75),  (gpointer) "Ontario");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (76),  (gpointer) "New Brunswick");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (77),  (gpointer) "Prince Edward Island");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (78),  (gpointer) "Nunavut");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (79),  (gpointer) "Québec");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (80),  (gpointer) "Nova Scotia");
      g_hash_table_insert (_divisions, GINT_TO_POINTER (81),  (gpointer) "Newfoundland");
    }

    {
      xmlDoc *doc = xmlParseFile (filename);

      if (doc)
      {
        xmlXPathInit ();

        {
          xmlXPathContext *context = xmlXPathNewContext (doc);

          xmlXPathRegisterNs (context, BAD_CAST "fred", BAD_CAST "http://www.askfred.net");

          if (CheckData (context))
          {
            LoadTournament (context);
            LoadClubs      (context);
            LoadFencers    (context);
            LoadEvents     (context);
          }

          xmlXPathFreeContext (context);
        }

        xmlFreeDoc (doc);
      }
    }
  }

  // --------------------------------------------------------------------------------
  Parser::~Parser ()
  {
    FreeFullGList (Event, _events);

    g_hash_table_unref (_fencers);
    g_hash_table_unref (_clubs);

    g_free (_name);
    g_free (_location);
    g_free (_organizer);
  }

  // --------------------------------------------------------------------------------
  GList *Parser::GetEvents ()
  {
    return _events;
  }

  // --------------------------------------------------------------------------------
  gboolean Parser::CheckData (xmlXPathContext *context )
  {
    gboolean        result     = FALSE;
    xmlXPathObject *xml_object;

    xml_object = xmlXPathEval (BAD_CAST "/fred:FencingData", context);
    if (xml_object->nodesetval->nodeNr == 1)
    {
      gchar      *attr;
      xmlNodeSet *xml_nodeset = xml_object->nodesetval;

      attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "Context");
      if (attr)
      {
        if (g_strcmp0 (attr, "PreReg") == 0)
        {
          result = TRUE;
        }
        xmlFree (attr);
      }
    }
    xmlXPathFreeObject  (xml_object);

    return result;
  }

  // --------------------------------------------------------------------------------
  void Parser::LoadTournament (xmlXPathContext *context )
  {
    xmlXPathObject *xml_object;

    xml_object = xmlXPathEval (BAD_CAST "/fred:FencingData/fred:Tournament", context);
    for (gint i = 0; i < xml_object->nodesetval->nodeNr; i++)
    {
      gchar      *attr;
      xmlNodeSet *xml_nodeset = xml_object->nodesetval;

      attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[i], BAD_CAST "Name");
      if (attr)
      {
        _name = g_strndup (attr, 30);
        if (strlen (attr) > 30)
        {
          g_strlcpy (&_name[27],
                     "...",
                     4);
        }

        xmlFree (attr);
      }

      attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[i], BAD_CAST "Org");
      if (attr)
      {
        _organizer = g_strdup (attr);
        xmlFree (attr);
      }

      attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[i], BAD_CAST "Location");
      if (attr)
      {
        _location = g_strdup (attr);
        xmlFree (attr);
      }
    }

    xmlXPathFreeObject  (xml_object);
  }

  // --------------------------------------------------------------------------------
  void Parser::LoadFencers (xmlXPathContext *context)
  {
    xmlXPathObject *xml_object;

    xml_object = xmlXPathEval (BAD_CAST "/fred:FencingData/fred:FencerDatabase/fred:Fencer", context);
    for (gint i = 0; i < xml_object->nodesetval->nodeNr; i++)
    {
      gchar      *attr;
      xmlNodeSet *xml_nodeset = xml_object->nodesetval;
      Fencer     *fencer = new Fencer ();

      attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[i], BAD_CAST "LastName");
      if (attr)
      {
        fencer->_last_name = g_strdup (attr);
        xmlFree (attr);
      }

      attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[i], BAD_CAST "FirstName");
      if (attr)
      {
        fencer->_first_name = g_strdup (attr);
        xmlFree (attr);
      }

      attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[i], BAD_CAST "Gender");
      if (attr)
      {
        fencer->_gender = g_strdup (attr);
        xmlFree (attr);
      }

      attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[i], BAD_CAST "BirthYear");
      if (attr)
      {
        fencer->_birth = g_strdup (attr);
        xmlFree (attr);
      }

      attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[i], BAD_CAST "ClubID1");
      if (attr)
      {
        guint  id   = g_ascii_strtoll (attr, NULL, 10);
        Club  *club;

        club = (Club *) g_hash_table_lookup (_clubs,
                                             GINT_TO_POINTER (id));
        fencer->_club = club;

        xmlFree (attr);
      }

      for (xmlNode *n = xml_nodeset->nodeTab[i]->children; n != NULL; n = n->next)
      {
        if (n->type == XML_ELEMENT_NODE)
        {
          if (g_strcmp0 ((gchar *)n->name, "Membership") == 0)
          {
            if (n->children && (n->children->type == XML_TEXT_NODE))
            {
              fencer->_licence = g_strdup ((gchar *) n->children->content);
            }
          }
          else if (g_strcmp0 ((gchar *)n->name, "Rating") == 0)
          {
            attr = (gchar *) xmlGetProp (n, BAD_CAST "Weapon");
            if (attr)
            {
              if (n->children && (n->children->type == XML_TEXT_NODE))
              {
                fencer->SetRanking (attr,
                                   (gchar *) n->children->content);
              }
              xmlFree (attr);
            }
          }
        }
      }

      attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[i], BAD_CAST "FencerID");
      if (attr)
      {
        g_hash_table_insert (_fencers,
                             GINT_TO_POINTER (g_ascii_strtoll (attr, NULL, 10)),
                             fencer);
        xmlFree (attr);
      }
    }

    xmlXPathFreeObject  (xml_object);
  }

  // --------------------------------------------------------------------------------
  void Parser::LoadClubs (xmlXPathContext *context)
  {
    xmlXPathObject *xml_object;

    xml_object = xmlXPathEval (BAD_CAST "/fred:FencingData/fred:ClubDatabase/fred:Club", context);
    for (gint i = 0; i < xml_object->nodesetval->nodeNr; i++)
    {
      gchar      *attr;
      xmlNodeSet *xml_nodeset = xml_object->nodesetval;
      Club       *club = new Club ();

      attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[i], BAD_CAST "Abbreviation");
      if (attr)
      {
        club->_name = g_strdup (attr);
        xmlFree (attr);
      }

      attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[i], BAD_CAST "DivisionID");
      if (attr)
      {
        guint id = g_ascii_strtoll (attr, NULL, 10);

        club->_division = g_strdup ((const gchar *) g_hash_table_lookup (_divisions,
                                                                         GINT_TO_POINTER (id)));
        xmlFree (attr);
      }

      attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[i], BAD_CAST "ClubID");
      if (attr)
      {
        g_hash_table_insert (_clubs,
                             GINT_TO_POINTER (g_ascii_strtoll (attr, NULL, 10)),
                             club);
        xmlFree (attr);
      }
    }

    xmlXPathFreeObject  (xml_object);
  }

  // --------------------------------------------------------------------------------
  void Parser::LoadEvents (xmlXPathContext *context)
  {
    xmlXPathObject *xml_object;

    xml_object = xmlXPathEval (BAD_CAST "/fred:FencingData/fred:Tournament/fred:Event", context);
    for (gint i = 0; i < xml_object->nodesetval->nodeNr; i++)
    {
      gchar      *attr;
      xmlNodeSet *xml_nodeset = xml_object->nodesetval;
      Event      *event       = new Event (_name,
                                           _location,
                                           _organizer);

      // Gender
      attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[i], BAD_CAST "Gender");
      if (attr)
      {
        if (g_strcmp0 (attr, "Women") == 0)
        {
          event->_gender = g_strdup ("F");
        }
        else if (g_strcmp0 (attr, "Men") == 0)
        {
          event->_gender = g_strdup ("M");
        }
        else
        {
          event->_gender = g_strdup ("FM");
        }

        xmlFree (attr);
      }

      // Weapon
      attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[i], BAD_CAST "Weapon");
      if (attr)
      {
        if (g_strcmp0 (attr, "Epee") == 0)
        {
          event->_weapon = g_strdup ("E");
        }
        else if (g_strcmp0 (attr, "Foil") == 0)
        {
          event->_weapon = g_strdup ("F");
        }
        else
        {
          event->_weapon = g_strdup ("S");
        }

        xmlFree (attr);
      }

      // AgeLimitMax
      attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[i], BAD_CAST "AgeLimitMax");
      if (attr)
      {
        if (g_strcmp0 (attr, "Y8") == 0)
        {
          event->_category = g_strdup ("M9");
        }
        else if (g_strcmp0 (attr, "Y10") == 0)
        {
          event->_category = g_strdup ("M11");
        }
        else if (g_strcmp0 (attr, "Y12") == 0)
        {
          event->_category = g_strdup ("M14");
        }
        else if (g_strcmp0 (attr, "Y14") == 0)
        {
          event->_category = g_strdup ("M14");
        }
        else if (g_strcmp0 (attr, "Cadet") == 0)
        {
          event->_category = g_strdup ("M17");
        }
        else if (g_strcmp0 (attr, "Junior") == 0)
        {
          event->_category = g_strdup ("M20");
        }
        else if (g_strcmp0 (attr, "Senior") == 0)
        {
          event->_category = g_strdup ("S");
        }
        else if (g_strcmp0 (attr, "Vet40") == 0)
        {
          event->_category = g_strdup ("V1");
        }
        else if (g_strcmp0 (attr, "Vet50") == 0)
        {
          event->_category = g_strdup ("V2");
        }
        else if (g_strcmp0 (attr, "Vet60") == 0)
        {
          event->_category = g_strdup ("V3");
        }
        else if (g_strcmp0 (attr, "Vet70") == 0)
        {
          event->_category = g_strdup ("V4");
        }
        else if (g_strcmp0 (attr, "VetCombined") == 0)
        {
          event->_category = g_strdup ("VET");
        }
        else
        {
          event->_category = g_strdup ("S");
        }

        xmlFree (attr);
      }

      // EventDateTime
      attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[i], BAD_CAST "EventDateTime");
      if (attr)
      {
        gchar **date = g_strsplit_set (attr,
                                       " -:",
                                       6);
        if (date[0] && date[1] && date[2] && date[3] && date[4])
        {
          event->_year   = g_ascii_strtoll (date[0], NULL, 10);
          event->_month  = g_ascii_strtoll (date[1], NULL, 10);
          event->_day    = g_ascii_strtoll (date[2], NULL, 10);
          event->_hour   = g_ascii_strtoll (date[3], NULL, 10);
          event->_minute = g_ascii_strtoll (date[4], NULL, 10);
        }

        g_strfreev (date);
        xmlFree (attr);
      }

      // Fencers
      for (xmlNode *n = xml_nodeset->nodeTab[i]->children; n != NULL; n = n->next)
      {
        if (n->type == XML_ELEMENT_NODE)
        {
          attr = (gchar *) xmlGetProp (n, BAD_CAST "CompetitorID");
          if (attr)
          {
            guint   id     = g_ascii_strtoll (attr, NULL, 10);
            Fencer *fencer;

            fencer = (Fencer *) g_hash_table_lookup (_fencers,
                                                     GINT_TO_POINTER (id));
            event->_fencers = g_list_prepend (event->_fencers,
                                              fencer);

            xmlFree (attr);
          }
        }
      }

      _events = g_list_prepend (_events,
                                event);
    }

    xmlXPathFreeObject  (xml_object);
  }
}
