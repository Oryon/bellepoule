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

#include "writer.hpp"

namespace AskFred
{
  namespace Writer
  {
    // --------------------------------------------------------------------------------
    IdFixer::IdFixer (const gchar *primary_title,
                      gboolean     generate_primary_id)
      : Object ("IdFixer")
    {
      _primary_title       = primary_title;
      _generate_primary_id = generate_primary_id;

      _population_count = 0;
      g_datalist_init (&_populations);
    }

    // --------------------------------------------------------------------------------
    IdFixer::~IdFixer ()
    {
      g_datalist_clear (&_populations);
    }

    // --------------------------------------------------------------------------------
    gboolean IdFixer::Knows (Element     *owner,
                             const gchar *name)
    {
      if (g_strcmp0 (owner->GetName (), "Tireur") == 0)
      {
        if (g_strcmp0 (name, _primary_title) == 0)
        {
          return TRUE;
        }
        else if (g_strcmp0 (name, "PluginID") == 0)
        {
          return TRUE;
        }
      }

      return FALSE;
    }

    // --------------------------------------------------------------------------------
    void IdFixer::PushId (const gchar *name,
                          const gchar *value)
    {
      if (GetId (value) == NULL)
      {
        if (g_strcmp0 (name, _primary_title) == 0)
        {
          gchar *default_askfred_id;

          if (_generate_primary_id)
          {
            if (GetId (value))
            {
              return;
            }
            else
            {
              _population_count++;
              default_askfred_id = g_strdup_printf ("-%d", _population_count);
            }
          }
          else
          {
            default_askfred_id = g_strdup_printf ("-%s", value);
          }

          g_datalist_set_data_full (&_populations,
                                    value,
                                    default_askfred_id,
                                    g_free);
        }
        else if (g_strcmp0 (name, "PluginID") == 0)
        {
          g_datalist_set_data_full (&_populations,
                                    value,
                                    g_strdup (value),
                                    g_free);
        }
      }
    }

    // --------------------------------------------------------------------------------
    const gchar *IdFixer::GetId (const gchar *of)
    {
      return (const gchar *) g_datalist_get_data (&_populations,
                                                  of);
    }

    // --------------------------------------------------------------------------------
    void IdFixer::Foreach (GDataForeachFunc  callback,
                           Object           *owner)
    {
      g_datalist_foreach (&_populations,
                          callback,
                          owner);
    }
  }

  namespace Writer
  {
    // --------------------------------------------------------------------------------
    Round::Round ()
      : Object ("Askfred::Round")
    {
      g_datalist_init (&_table);
    }

    // --------------------------------------------------------------------------------
    Round::~Round ()
    {
      g_datalist_clear (&_table);
    }

    // --------------------------------------------------------------------------------
    void Round::Delete (Round *round)
    {
      round->Release ();
    }

    // --------------------------------------------------------------------------------
    void Round::Withdraw (const gchar *id,
                          gpointer     data)
    {
      g_datalist_set_data (&_table,
                           id,
                           data);
    }

    // --------------------------------------------------------------------------------
    gboolean Round::IsWithdrawn (const gchar *id)
    {
      return g_datalist_get_data (&_table,
                                  id) != NULL;
    }
  }

  namespace Writer
  {
    // --------------------------------------------------------------------------------
    Element::Element (const gchar *name,
                      GData       *scheme,
                      Round       *round)
      : Object ("AskFred::Element")
    {
      _name    = g_strdup (name);
      _ref     = NULL;
      _scheme  = scheme;
      _seq     = 0;
      _visible = TRUE;
      _outline = NULL;
      _round   = NULL;

      if (round)
      {
        round->Retain ();
        _round = round;
      }
    }

    // --------------------------------------------------------------------------------
    Element::~Element ()
    {
      g_free (_name);

      Object::TryToRelease (_round);

      if (_outline)
      {
        g_list_free_full (_outline,
                          g_free);
      }
    }

    // --------------------------------------------------------------------------------
    gboolean Element::DeleteNode (GNode    *node,
                                  gpointer  data)
    {
      Element *element = (Element *) node->data;

      element->Release ();
      return FALSE;
    }

    // --------------------------------------------------------------------------------
    void Element::MakeInvisible ()
    {
      _visible = FALSE;
    }

    // --------------------------------------------------------------------------------
    GList *Element::GetOutline ()
    {
      if (_visible)
      {
        return _outline;
      }

      return NULL;
    }

    // --------------------------------------------------------------------------------
    const gchar *Element::GetName ()
    {
      return _name;
    }

    // --------------------------------------------------------------------------------
    GData *Element::GetScheme ()
    {
      return _scheme;
    }

    // --------------------------------------------------------------------------------
    void Element::CountSeq ()
    {
      _seq++;
    }

    // --------------------------------------------------------------------------------
    void Element::CancelSeq ()
    {
      _seq--;
    }

    // --------------------------------------------------------------------------------
    guint Element::GetSeq ()
    {
      return _seq;
    }

    // --------------------------------------------------------------------------------
    void Element::Withdraw ()
    {
      if (_round)
      {
        _round->Withdraw (_ref,
                          this);
      }
    }

    // --------------------------------------------------------------------------------
    gboolean Element::IsWithdrawn ()
    {
      if (_round)
      {
        return _round->IsWithdrawn (_ref);
      }

      return FALSE;
    }

    // --------------------------------------------------------------------------------
    void Element::StartOutline (const gchar *name)
    {
      _outline = g_list_append (_outline,
                                g_strdup (name));
    }

    // --------------------------------------------------------------------------------
    void Element::AddToOutline (const gchar *name,
                                const gchar *value)
    {
      gchar *copied_value = g_strdup (value);
      GList *new_name     = g_list_find_custom (_outline,
                                                name,
                                                (GCompareFunc) g_strcmp0);
      if (new_name)
      {
        GList *old_value = g_list_next (new_name);

        _outline = g_list_insert_before (_outline,
                                         old_value,
                                         copied_value);

        _outline = g_list_remove_link (_outline,
                                       old_value);
        g_list_free_full (old_value,
                          g_free);
      }
      else
      {
        _outline = g_list_append (_outline,
                                  g_strdup (name));
        _outline = g_list_append (_outline,
                                  copied_value);
      }

      if (g_strcmp0 (name, "REF") == 0)
      {
        _ref = copied_value;
      }
    }
  }

  namespace Writer
  {
    GData *Scheme::_element_base = NULL;

    // --------------------------------------------------------------------------------
    Scheme::Scheme (const gchar *filename)
      : Object ("AskFred::XmlScheme"),
      XmlScheme (filename)
    {
      _mode         = COLLECTING_MODE;
      _current_node = NULL;
      _rounds       = NULL;

      _fencer_id_fixer = new IdFixer ("ID",   FALSE);
      _club_id_fixer   = new IdFixer ("Club", TRUE);

      if (_element_base == NULL)
      {
        g_datalist_init (&_element_base);

        // FencingData
        {
          GData *scheme;

          g_datalist_init (&scheme);

          g_datalist_set_data (&scheme, "Tireurs",         (gpointer) "FencerDatabase");
          g_datalist_set_data (&scheme, "Tireur",          (gpointer) "Fencer");
          g_datalist_set_data (&scheme, "Arbitres",        (gpointer) "RefereeDatabase");
          g_datalist_set_data (&scheme, "Arbitre",         (gpointer) "Referee");
          g_datalist_set_data (&scheme, "Equipes",         (gpointer) "TeamDatabase");
          g_datalist_set_data (&scheme, "Equipe",          (gpointer) "Team");

          g_datalist_set_data (&scheme, "Nom",             (gpointer) "FirstName");
          g_datalist_set_data (&scheme, "Prenom",          (gpointer) "LastName");
          g_datalist_set_data (&scheme, "DateNaissance",   (gpointer) "BirthYear");
          g_datalist_set_data (&scheme, "Sexe",            (gpointer) "Gender");
          g_datalist_set_data (&scheme, "Club",            (gpointer) "ClubID");
          g_datalist_set_data (&scheme, "Classement",      (gpointer) "Place");

          g_datalist_set_data (&scheme, "Phases",          (gpointer) "RoundResults");
          g_datalist_set_data (&scheme, "Poule",           (gpointer) "Pool");
          g_datalist_set_data (&scheme, "TourDePoules",    (gpointer) "Round");
          g_datalist_set_data (&scheme, "PhaseDeTableaux", (gpointer) "Round");
          g_datalist_set_data (&scheme, "REF",             (gpointer) "CompetitorID");
          g_datalist_set_data (&scheme, "RangInitial",     (gpointer) "Seed");
          g_datalist_set_data (&scheme, "Statut",          (gpointer) "Status");
          g_datalist_set_data (&scheme, "Q",               (gpointer) "P");
          g_datalist_set_data (&scheme, "A",               (gpointer) "W");

          g_datalist_set_data (&scheme, "NoDansLaPoule",   (gpointer) "CompetitorNum");
          g_datalist_set_data (&scheme, "NbMatches",       (gpointer) "Bouts");
          g_datalist_set_data (&scheme, "NbVictoires",     (gpointer) "Victories");
          g_datalist_set_data (&scheme, "TD",              (gpointer) "TS");
          g_datalist_set_data (&scheme, "RangPoule",       (gpointer) "Rank");

          g_datalist_set_data (&scheme, "Tableau",         (gpointer) "Table");
          g_datalist_set_data (&scheme, "Taille",          (gpointer) "Of");

          g_datalist_set_data (&scheme, "Match",           (gpointer) "Bout");

          g_datalist_set_data (&_element_base,
                               "FencingData",
                               scheme);
        }

        // Tournament
        {
          GData *scheme;

          g_datalist_init (&scheme);

          g_datalist_set_data (&scheme, "Tireurs", (gpointer) "FinalResults");
          g_datalist_set_data (&scheme, "Equipes", (gpointer) "FinalResults");
          g_datalist_set_data (&scheme, "Tireur",  (gpointer) "Result");
          g_datalist_set_data (&scheme, "Equipe",  (gpointer) "Result");
          g_datalist_set_data (&scheme, "M",       (gpointer) "Men");
          g_datalist_set_data (&scheme, "F",       (gpointer) "Female");
          g_datalist_set_data (&scheme, "FM",      (gpointer) "Mixed");
          g_datalist_set_data (&scheme, "Sabre",   (gpointer) "Saber");
          g_datalist_set_data (&scheme, "Epée",    (gpointer) "Epee");

          g_datalist_set_data (&_element_base,
                               "Tournament",
                               scheme);
        }

        // Phases
        {
          GData *scheme;

          g_datalist_init (&scheme);

          g_datalist_set_data (&scheme, "Tireur", (gpointer) "Competitor");
          g_datalist_set_data (&scheme, "Equipe", (gpointer) "Competitor");

          g_datalist_set_data (&_element_base,
                               "Phases",
                               scheme);
        }

        // Tableau/Match/Tireur
        {
          GData *scheme;

          g_datalist_init (&scheme);

          g_datalist_set_data (&scheme, "A", (gpointer) "D");
          g_datalist_set_data (&scheme, "E", (gpointer) "D");

          g_datalist_set_data (&_element_base,
                               "Tableau/Match/Tireur",
                               scheme);
        }

        // Event/Tireurs
        // Event/Equipes
        {
          GData *scheme;

          g_datalist_init (&scheme);

          g_datalist_set_data (&scheme, "PluginID", (gpointer) "CompetitorID");
          g_datalist_set_data (&scheme, "ID",       (gpointer) "CompetitorID");

          g_datalist_set_data (&_element_base,
                               "Event/Tireurs",
                               scheme);
          g_datalist_set_data (&_element_base,
                               "Event/Equipes",
                               scheme);
        }

        // FencingData/Tireurs
        {
          GData *scheme;

          g_datalist_init (&scheme);

          g_datalist_set_data (&scheme, "PluginID", (gpointer) "FencerID");
          g_datalist_set_data (&scheme, "ID",       (gpointer) "FencerID");
          g_datalist_set_data (&scheme, "Club",     (gpointer) "clubID1");

          g_datalist_set_data (&_element_base,
                               "FencingData/Tireurs",
                               scheme);
        }

        // Equipes/Tireur
        {
          GData *scheme;

          g_datalist_init (&scheme);

          g_datalist_set_data (&scheme, "PluginID", (gpointer) "FencerID");
          g_datalist_set_data (&scheme, "ID",       (gpointer) "FencerID");

          g_datalist_set_data (&_element_base,
                               "Equipe/Tireur",
                               scheme);
        }

        // FencingData/Equipes
        {
          GData *scheme;

          g_datalist_init (&scheme);

          g_datalist_set_data (&scheme, "PluginID", (gpointer) "TeamID");
          g_datalist_set_data (&scheme, "ID",       (gpointer) "TeamID");

          g_datalist_set_data (&_element_base,
                               "FencingData/Equipes",
                               scheme);
        }

        // Arbitre
        {
          GData *scheme;

          g_datalist_init (&scheme);

          g_datalist_set_data (&scheme, "ID", (gpointer) "RefereeID");

          g_datalist_set_data (&_element_base,
                               "Arbitre",
                               scheme);
        }

        // Club
        {
          GData *scheme;

          g_datalist_init (&scheme);

          g_datalist_set_data (&scheme, "Club", (gpointer) "Club");

          g_datalist_set_data (&_element_base,
                               "Club",
                               scheme);
        }
      }
    }

    // --------------------------------------------------------------------------------
    Scheme::~Scheme ()
    {
      g_list_free_full (_rounds,
                        (GDestroyNotify) Round::Delete);

      _fencer_id_fixer->Release ();
      _club_id_fixer->Release   ();

      g_node_traverse (_root_element,
                       G_PRE_ORDER,
                       G_TRAVERSE_ALL,
                       -1,
                       Element::DeleteNode,
                       NULL);

      g_node_destroy (_root_element);
    }

    // --------------------------------------------------------------------------------
    Element *Scheme::PushElement (const gchar *name)
    {
      GData   *scheme  = NULL;
      Element *element;

      // Get the proper scheme
      {
        GList *chain = NULL;

        for (GNode *from = _current_node; from != NULL; from = from->parent)
        {
          chain = g_list_prepend (chain, from->data);
        }

        for (GList *from = chain; from != NULL; from = g_list_next (from))
        {
          GString *path    = g_string_new ("");
          GList   *current = from;

          while (current)
          {
            Element *node = (Element *) current->data;

            g_string_append   (path, node->GetName ());
            g_string_append_c (path, '/');

            current = g_list_next (current);
          }
          g_string_append (path, name);

          scheme = (GData *) g_datalist_get_data (&_element_base,
                                                  path->str);
          g_string_free (path,
                         TRUE);

          if (scheme)
          {
            break;
          }
        }

        g_list_free (chain);

        if (scheme == NULL)
        {
          scheme = (GData *) g_datalist_get_data (&_element_base,
                                                  name);
        }
      }

      // Creation & Stacking
      {
        if (_rounds)
        {
          element = new Element (name,
                                 scheme,
                                 (Round *) _rounds->data);
        }
        else
        {
          element = new Element (name,
                                 scheme,
                                 NULL);
        }

        if (_current_node)
        {
          _current_node = g_node_append_data (_current_node,
                                              element);
        }
        else
        {
          _current_node = g_node_new (element);
          _root_element = _current_node;
        }
      }


      // Check visibility
      if (   (g_strcmp0 (name, "SuiteDeTableaux")   == 0)
          || (g_strcmp0 (name, "Pointage")          == 0)
          || (g_strcmp0 (name, "ClassementGeneral") == 0))
      {
        element->MakeInvisible ();
      }
      else
      {
        GNode *parent = _current_node->parent;

        if (parent)
        {
          GNode *grand_parent = parent->parent;

          if (grand_parent)
          {
            Element *parent_element       = (Element *) parent->data;
            Element *grand_parent_element = (Element *) grand_parent->data;

            if (   (g_strcmp0 (parent_element->GetName (),                   "Equipe")       == 0)
                && (g_strcmp0 (Translate (grand_parent_element->GetName ()), "FinalResults") == 0))
            {
              if (g_strcmp0 (name, "Tireur") == 0)
              {
                element->MakeInvisible ();
              }
            }
          }
        }
      }

      return element;
    }

    // --------------------------------------------------------------------------------
    gboolean Scheme::SaveFencersAndTeamsSeparatly ()
    {
      return TRUE;
    }

    // --------------------------------------------------------------------------------
    const gchar *Scheme::Translate (const gchar *term)
    {
      GNode *current = _current_node;

      while (current)
      {
        Element *element = (Element *) current->data;
        GData   *scheme  = element->GetScheme ();

        if (scheme)
        {
          const gchar *translation = (const gchar *) g_datalist_get_data (&scheme,
                                                                          term);
          if (translation)
          {
            return translation;
          }
        }

        current = current->parent;
      }

      return term;
    }

    // --------------------------------------------------------------------------------
    const gchar *Scheme::TranslateVValue (const gchar *term,
                                          const gchar *format_value,
                                          va_list      argptr)
    {
      if (_mode == COLLECTING_MODE)
      {
        Element *element = (Element *) _current_node->data;
        gchar   *value   = g_strdup_vprintf (format_value,
                                             argptr);

        element->AddToOutline (term,
                               value);

        g_free (value);
        return NULL;
      }

      return format_value;
    }

    // --------------------------------------------------------------------------------
    const gchar *Scheme::TranslateValue (const gchar *term,
                                         const gchar *value)
    {
      Element *owner  = (Element *) _current_node->data;
      Element *parent = NULL;

      if (_current_node->parent)
      {
        parent = (Element *) _current_node->parent->data;
      }

      if (_mode == COLLECTING_MODE)
      {
        owner->AddToOutline (term,
                             value);

        if (parent)
        {
          if (   (g_strcmp0 (Translate (parent->GetName ()), "FinalResults") == 0)
              && (g_strcmp0 (term,  "Statut") == 0))
          {
            if (g_strcmp0 (value, "E") == 0)
            {
              owner->AddToOutline ("Excluded",
                                   "True");
            }
            else
            {
              owner->AddToOutline ("Excluded",
                                   "False");
            }

            if (g_strcmp0 (value, "A") == 0)
            {
              owner->AddToOutline ("DNF",
                                   "True");
            }
            else
            {
              owner->AddToOutline ("DNF",
                                   "False");
            }
          }
          else if (   (g_strcmp0 (parent->GetName (), "RoundSeeding") == 0)
                   && (g_strcmp0 (term, "Statut") == 0))
          {
            if (   (g_strcmp0 (value, "A") == 0)
                || (g_strcmp0 (value, "E") == 0))
            {
              owner->Withdraw ();
            }
          }

          if (_fencer_id_fixer->Knows (owner, term))
          {
            _fencer_id_fixer->PushId (term,
                                      value);
          }
          else if (_club_id_fixer->Knows (owner, term))
          {
            _club_id_fixer->PushId (term,
                                    value);
          }
        }

        return NULL;
      }

      if (g_strcmp0 (term, "DateNaissance") == 0)
      {
        gchar *year = g_strrstr (value,
                                 ".");

        if (year)
        {
          return year+1;
        }
      }
      else if (   parent
               && (g_strcmp0 (parent->GetName (), "RoundSeeding") == 0)
               && (g_strcmp0 (term, "Statut") == 0))
      {
        if (owner->IsWithdrawn ())
        {
          return "P";
        }
      }

      if (   (g_strcmp0 (term, "Statut") == 0)
          || (g_strcmp0 (term, "Gender") == 0)
          || (g_strcmp0 (term, "Weapon") == 0))
      {
        return Translate (value);
      }

      RemoveNonBreakingSpaces ((guchar *) value);
      return value;
    }

    // --------------------------------------------------------------------------------
    void Scheme::RemoveNonBreakingSpaces (guchar *in)
    {
      guint len = strlen ((gchar *) in);

      if (len > 2)
      {
        guint n_space = 0;
        guint c       = 0;

        for (c = 0; c < len; c++)
        {
          if ((in[c] == 0xC2) && (in[c+1] == 0xA0))
          {
            in[c-n_space] = ' ';
            c++;
            n_space++;
          }
          else if (n_space)
          {
            in[c-n_space] = in[c];
          }
        }

        if (n_space)
        {
          in[c-n_space] = '\0';
        }
      }
    }

    // --------------------------------------------------------------------------------
    void Scheme::StartElement (const gchar *name)
    {
      Element *parent = NULL;

      // Round seeding
      if (_current_node)
      {
        parent = (Element *) _current_node->data;

        // Implicit start
        if (   ((g_strcmp0 (name, "Tireur") == 0) || (g_strcmp0 (name, "Equipe") == 0))
            && (g_strcmp0 (Translate (parent->GetName ()), "Round")  == 0))
        {
          _rounds = g_list_prepend (_rounds,
                                    new Round ());

          StartElement ("RoundSeeding");
        }

        // Implicit close
        if (   (g_strcmp0 (name, "Poule") == 0)
            || (g_strcmp0 (name, "SuiteDeTableaux") == 0))
        {
          EndElement ("RoundSeeding");
        }
      }

      PushElement (name);

      {
        Element *owner = (Element *) _current_node->data;

        owner->StartOutline (name);

        if (parent && g_strcmp0 (parent->GetName (), "Match") == 0)
        {
          owner->AddToOutline ("Score",
                               "0");
          owner->AddToOutline ("Statut",
                               "V");
        }
      }
    }

    // --------------------------------------------------------------------------------
    gboolean Scheme::FlushElement (Element *element,
                                   Element *parent)
    {
      GList *outline = element->GetOutline ();

      if (outline)
      {
        XmlScheme::StartElement ((const gchar *) outline->data);

        outline = g_list_next (outline);

        while (outline)
        {
          const gchar *term;
          const gchar *value;
          gchar       *value_to_write = NULL;

          term  = (const gchar *) outline->data;
          outline = g_list_next (outline);

          value = (const gchar *) outline->data;
          outline = g_list_next (outline);

          if (   (g_strcmp0 (term, "REF") == 0)
              || (g_strcmp0 (term, "ID")  == 0))
          {
            value_to_write = g_strdup (_fencer_id_fixer->GetId (value));
          }
          else if (g_strcmp0 (term, "Club") == 0)
          {
            value_to_write = g_strdup (_club_id_fixer->GetId (value));
          }
          else if (g_strcmp0 (term, "NoDansLaPoule") == 0)
          {
            value_to_write = g_strdup_printf ("%d", parent->GetSeq ());
          }

          if (value_to_write == NULL)
          {
            value_to_write = g_strdup (value);
          }

          WriteAttribute (term,
                          value_to_write);
          g_free (value_to_write);
        }
        return TRUE;
      }

      return FALSE;
    }

    // --------------------------------------------------------------------------------
    gboolean Scheme::FlushNode (GNode *node)
    {
      {
        Element *element = (Element *) node->data;
        Element *parent  = NULL;

        _current_node = node;

        if (node->parent)
        {
          parent = (Element *) node->parent->data;

          if (element->GetOutline ())
          {
            if (   (g_strcmp0 (element->GetName (), "Tireur")          == 0)
                || (g_strcmp0 (element->GetName (), "Equipe")          == 0)
                || (g_strcmp0 (element->GetName (), "TourDePoules")    == 0)
                || (g_strcmp0 (element->GetName (), "Poule")           == 0)
                || (g_strcmp0 (element->GetName (), "PhaseDeTableaux") == 0)
                || (g_strcmp0 (element->GetName (), "Tableau")         == 0))
            {
              parent->CountSeq ();
            }
          }
        }

        if (FlushElement (element,
                          parent))
        {
          if (parent)
          {
            if (g_strcmp0 (element->GetName (), "TourDePoules") == 0)
            {
              WriteAttribute ("Type",
                              "Pool");
              WriteAttribute ("Desc",
                              "Pools");
              WriteFormatAttribute ("Seq",
                                    "%d", parent->GetSeq ());
            }
            else if (g_strcmp0 (element->GetName (), "Poule") == 0)
            {
              WriteFormatAttribute ("Seq",
                                    "%d", parent->GetSeq ());
            }
            else if (g_strcmp0 (element->GetName (), "PhaseDeTableaux") == 0)
            {
              WriteAttribute ("Type",
                              "DE");
              WriteAttribute ("Desc",
                              "Tables");
              WriteFormatAttribute ("Seq",
                                    "%d", parent->GetSeq ());
            }
            else if (g_strcmp0 (element->GetName (), "Tableau") == 0)
            {
              WriteFormatAttribute ("Seq",
                                    "%d", parent->GetSeq ());
            }
            else if (g_strcmp0 (parent->GetName (), "Match") == 0)
            {
              WriteFormatAttribute ("Called",
                                    "%d", parent->GetSeq ());
            }
          }
        }
      }

      if (_current_node->children == NULL)
      {
        while (_current_node)
        {
          Element *element = (Element *) _current_node->data;

          if (element->GetOutline ())
          {
            if (g_strcmp0 (element->GetName (), "Match") == 0)
            {
              if (element->GetSeq () < 2)
              {
                XmlScheme::StartElement ("Bye");
                XmlScheme::EndElement ();
              }
            }

            XmlScheme::EndElement ();
          }

          if (_current_node->next == NULL)
          {
            _current_node = _current_node->parent;
          }
          else
          {
            break;
          }
        }
      }

      return FALSE;
    }

    // --------------------------------------------------------------------------------
    gboolean Scheme::CheckVisibilityNodeFunc (GNode  *node,
                                              Scheme *scheme)
    {
      Element *element = (Element *) node->data;

      if (element->IsWithdrawn ())
      {
        GNode   *parent_node = node->parent;
        Element *parent      = (Element *) parent_node->data;

        if (g_strcmp0 (parent->GetName (), "Poule") == 0)
        {
          element->MakeInvisible ();
        }
        else if (g_strcmp0 (parent->GetName (), "Match") == 0)
        {
          GNode   *grand_parent_node = parent_node->parent;
          Element *grand_parent      = (Element *) grand_parent_node->data;

          if (g_strcmp0 (grand_parent->GetName (), "Poule") == 0)
          {
            GNode *opponent_node = parent_node->children;

            while (opponent_node)
            {
              Element *opponent = (Element *) opponent_node->data;

              opponent->MakeInvisible ();

              opponent_node = opponent_node->next;
            }

            parent->MakeInvisible ();
          }
        }
      }

      return FALSE;
    }

    // --------------------------------------------------------------------------------
    gboolean Scheme::FlushNodeFunc (GNode  *node,
                                    Scheme *scheme)
    {
      return scheme->FlushNode (node);
    }

    // --------------------------------------------------------------------------------
    void Scheme::Flush (GNode *from)
    {
      _mode = POST_COLLECTING_MODE;
      g_node_traverse (from,
                       G_PRE_ORDER,
                       G_TRAVERSE_LEAVES,
                       -1,
                       (GNodeTraverseFunc) CheckVisibilityNodeFunc,
                       this);

      _mode = FLUSHING_MODE;
      g_node_traverse (from,
                       G_PRE_ORDER,
                       G_TRAVERSE_ALL,
                       -1,
                       (GNodeTraverseFunc) FlushNodeFunc,
                       this);
    }

    // --------------------------------------------------------------------------------
    void Scheme::EndElement ()
    {
      if (_mode == COLLECTING_MODE)
      {
        _current_node = _current_node->parent;

        if (_current_node == NULL)
        {
          Flush (_root_element);
        }
      }
    }

    // --------------------------------------------------------------------------------
    void Scheme::EndElement (const gchar *name)
    {
      Element *element = (Element *) _current_node->data;

      if (g_strcmp0 (element->GetName (), name) == 0)
      {
        EndElement ();
      }
    }

    // --------------------------------------------------------------------------------
    void Scheme::WriteCustom (const gchar *what)
    {
      StartElement ("ClubDatabase");
      _club_id_fixer->Foreach ((GDataForeachFunc) WriteClub,
                               this);
      EndElement ();
    }

    // --------------------------------------------------------------------------------
    void Scheme::WriteClub (GQuark  quark,
                            gchar  *id,
                            Object *owner)
    {
      Scheme *scheme = dynamic_cast <Scheme *> (owner);

      scheme->StartElement ("Club");
      scheme->WriteAttribute ("ClubID",       id);
      scheme->WriteAttribute ("Name",         g_quark_to_string (quark));
      scheme->WriteAttribute ("Abbreviation", g_quark_to_string (quark));
      scheme->EndElement ();
    }
  }
}
