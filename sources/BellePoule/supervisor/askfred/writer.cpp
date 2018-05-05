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

      _current_id = NULL;
    }

    // --------------------------------------------------------------------------------
    IdFixer::~IdFixer ()
    {
      g_datalist_clear (&_populations);
      g_free (_current_id);
    }

    // --------------------------------------------------------------------------------
    gboolean IdFixer::Knows (Element     *owner,
                             const gchar *id)
    {
      if (g_strcmp0 (owner->GetName (), "Tireur") == 0)
      {
        if (g_strcmp0 (id, _primary_title) == 0)
        {
          return TRUE;
        }
        else if (_current_id && (g_strcmp0 (id, "PluginID") == 0))
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
      if (g_strcmp0 (name, _primary_title) == 0)
      {
        gchar *default_askfred_id;

        _current_id = g_strdup (value);

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
                                  _current_id,
                                  default_askfred_id,
                                  g_free);
      }
      else if (_current_id && (g_strcmp0 (name, "PluginID") == 0))
      {
        g_datalist_set_data_full (&_populations,
                                  _current_id,
                                  g_strdup (value),
                                  g_free);
      }
    }

    // --------------------------------------------------------------------------------
    const gchar *IdFixer::PopId ()
    {
      const gchar *fixed_id = NULL;

      if (_current_id)
      {
        fixed_id = (const gchar *) g_datalist_get_data (&_populations,
                                                        _current_id);
        g_free (_current_id);
        _current_id = NULL;
      }

      return fixed_id;
    }

    // --------------------------------------------------------------------------------
    const gchar *IdFixer::GetId (const gchar *of)
    {
      return (const gchar *) g_datalist_get_data (&_populations,
                                                  of);
    }

    // --------------------------------------------------------------------------------
    const gchar *IdFixer::GetPrimaryTitle ()
    {
      return _primary_title;
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
    Element::Element (const gchar *name,
                      GData       *scheme)
      : Object ("AskFred::Element")
    {
      _name    = name;
      _scheme  = scheme;
      _seq     = 0;
      _visible = TRUE;
    }

    // --------------------------------------------------------------------------------
    Element::~Element ()
    {
    }

    // --------------------------------------------------------------------------------
    void Element::SetInvisible ()
    {
      _visible = FALSE;
    }

    // --------------------------------------------------------------------------------
    gboolean Element::IsVisible ()
    {
      return _visible;
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
    guint Element::GetSeq ()
    {
      return _seq;
    }
  }

  namespace Writer
  {
    GList *Scheme::_element_stack = NULL;
    GData *Scheme::_element_base  = NULL;

    // --------------------------------------------------------------------------------
    Scheme::Scheme (const gchar *filename)
      : Object ("AskFred::XmlScheme"),
      XmlScheme (filename)
    {
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

        // Equipes/Tireurs
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
      _fencer_id_fixer->Release ();
      _club_id_fixer->Release ();
    }

    // --------------------------------------------------------------------------------
    void Scheme::PushElement (const gchar *name)
    {
      GData *scheme = NULL;

      {
        GList *from = g_list_last (_element_stack);

        while (from)
        {
          GString *path    = g_string_new ("");
          GList   *current = from;

          while (current)
          {
            Element *node = (Element *) current->data;

            g_string_append   (path, node->GetName ());
            g_string_append_c (path, '/');

            current = g_list_previous (current);
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

          from = g_list_previous (from);
        }

        if (scheme == NULL)
        {
          scheme = (GData *) g_datalist_get_data (&_element_base,
                                                  name);
        }
      }

      {
        Element *element = new Element (name,
                                        scheme);

        _element_stack = g_list_prepend (_element_stack,
                                         element);

        {
          Element *parent       = (Element *) g_list_nth_data (_element_stack, 1);
          Element *grand_parent = (Element *) g_list_nth_data (_element_stack, 2);

          if (parent && grand_parent
              && (g_strcmp0 (parent->GetName (),                   "Equipe")       == 0)
              && (g_strcmp0 (Translate (grand_parent->GetName ()), "FinalResults") == 0))
          {
            if (g_strcmp0 (name, "Tireur") == 0)
            {
              element->SetInvisible ();
              return;
            }
          }
        }

        if (   (g_strcmp0 (name, "SuiteDeTableaux")   == 0)
            || (g_strcmp0 (name, "Pointage")          == 0)
            || (g_strcmp0 (name, "ClassementGeneral") == 0))
        {
          element->SetInvisible ();
        }
      }
    }

    // --------------------------------------------------------------------------------
    gboolean Scheme::SaveFencersAndTeamsSeparatly ()
    {
      return TRUE;
    }

    // --------------------------------------------------------------------------------
    gboolean Scheme::CurrentElementIsVisible ()
    {
      Element *current_element = (Element *) _element_stack->data;

      return current_element->IsVisible ();
    }

    // --------------------------------------------------------------------------------
    const gchar *Scheme::Translate (const gchar *term)
    {
      GList *current = _element_stack;

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

        current = g_list_next (current);
      }

      return term;
    }

    // --------------------------------------------------------------------------------
    const gchar *Scheme::TranslateVValue (const gchar *term,
                                          const gchar *format_value,
                                          va_list      argptr)
    {
      if (g_strcmp0 (term, "REF") == 0)
      {
        const gchar *askfred_id;
        gchar       *ref = g_strdup_vprintf (format_value,
                                             argptr);

        askfred_id = _fencer_id_fixer->GetId (ref);
        g_free (ref);

        if (askfred_id)
        {
          return askfred_id;
        }
      }

      return format_value;
    }

    // --------------------------------------------------------------------------------
    const gchar *Scheme::TranslateValue (const gchar *term,
                                         const gchar *value)
    {
      {
        Element *owner= (Element *) g_list_nth_data (_element_stack, 0);

        if (_fencer_id_fixer->Knows (owner, term))
        {
          _fencer_id_fixer->PushId (term,
                                    value);
          return NULL;
        }
        else if (_club_id_fixer->Knows (owner, term))
        {
          _club_id_fixer->PushId (term,
                                  value);
          return NULL;
        }
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

      if (_element_stack)
      {
        // Round seeding
        {
          parent = (Element *) _element_stack->data;

          // Implicit start
          if (   ((g_strcmp0 (name, "Tireur") == 0) || (g_strcmp0 (name, "Equipe") == 0))
              && (g_strcmp0 (Translate (parent->GetName ()), "Round")  == 0))
          {
            StartElement ("RoundSeeding");
          }

          // Implicit close
          if (   (g_strcmp0 (name, "Poule") == 0)
              || (g_strcmp0 (name, "SuiteDeTableaux") == 0))
          {
            EndElement ("RoundSeeding");
          }
        }

        parent = (Element *) _element_stack->data;

        // Seq
        if (   (g_strcmp0 (name, "Tireur")          == 0)
            || (g_strcmp0 (name, "Equipe")          == 0)
            || (g_strcmp0 (name, "TourDePoules")    == 0)
            || (g_strcmp0 (name, "Poule")           == 0)
            || (g_strcmp0 (name, "PhaseDeTableaux") == 0)
            || (g_strcmp0 (name, "Tableau")         == 0))
        {
          parent->CountSeq ();
        }
      }

      PushElement (name);
      XmlScheme::StartElement (name);

      // Extra attributes
      if (g_strcmp0 (name, "TourDePoules") == 0)
      {
        WriteAttribute ("Type",
                        "Pool");
        WriteAttribute ("Desc",
                        "Pools");
        WriteFormatAttribute ("Seq",
                              "%d", parent->GetSeq ());
      }
      else if (g_strcmp0 (name, "Poule") == 0)
      {
        WriteFormatAttribute ("Seq",
                              "%d", parent->GetSeq ());
      }
      else if (g_strcmp0 (name, "PhaseDeTableaux") == 0)
      {
        WriteAttribute ("Type",
                        "DE");
        WriteAttribute ("Desc",
                        "Tables");
        WriteFormatAttribute ("Seq",
                              "%d", parent->GetSeq ());
      }
      else if (g_strcmp0 (name, "Tableau") == 0)
      {
        WriteFormatAttribute ("Seq",
                              "%d", parent->GetSeq ());
      }
    }


    // --------------------------------------------------------------------------------
    void Scheme::WritePendingId (IdFixer *id_fixer)
    {
      const gchar *pending_id = id_fixer->PopId ();

      if (pending_id)
      {
        WriteAttribute (Translate (id_fixer->GetPrimaryTitle ()),
                        pending_id);
      }
    }

    // --------------------------------------------------------------------------------
    void Scheme::EndElement ()
    {
      Element *element = (Element *) _element_stack->data;

      if (g_strcmp0 (element->GetName (), "Match") == 0)
      {
        if (element->GetSeq () < 2)
        {
          StartElement ("Bye");
          EndElement ();
        }
      }

      WritePendingId (_fencer_id_fixer);
      WritePendingId (_club_id_fixer);

      XmlScheme::EndElement ();

      {
        _element_stack = g_list_delete_link (_element_stack,
                                             _element_stack);
        element->Release ();
      }
    }

    // --------------------------------------------------------------------------------
    void Scheme::EndElement (const gchar *name)
    {
      Element *element = (Element *) _element_stack->data;

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
