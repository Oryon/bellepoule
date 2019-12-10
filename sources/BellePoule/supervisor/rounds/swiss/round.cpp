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

#include "util/data.hpp"
#include "util/glade.hpp"
#include "util/attribute_desc.hpp"
#include "util/filter.hpp"
#include "util/player.hpp"
#include "util/xml_scheme.hpp"
#include "../../match.hpp"

#include "round.hpp"

namespace Swiss
{
  const gchar *Round::_class_name     = N_ ("Pool");
  const gchar *Round::_xml_class_name = "RondeSuisse";

  // --------------------------------------------------------------------------------
  Round::Round (StageClass *stage_class)
    : Object ("Swiss::Round"),
      Stage (stage_class),
      CanvasModule ("swiss_supervisor.glade")
  {
    _matches = nullptr;

    _max_score = new Data ("ScoreMax",
                           15);

    {
      _nb_qualified->Release ();
      _nb_qualified = new Data ("NbQualifiesParIndice",
                                8);
      ActivateNbQualified ();
    }

    _matches_per_fencer = new Data ("MatchsParTireur",
                                    7);

    {
      GSList *attr_list;
      Filter *filter;

      AttributeDesc::CreateExcludingList (&attr_list,
#ifndef DEBUG
                                          "ref",
                                          "plugin_ID",
#endif
                                          "incident",
                                          "IP",
                                          "password",
                                          "cyphered_password",
                                          "HS",
                                          "attending",
                                          "exported",
                                          "final_rank",
                                          "global_status",
                                          "indice",
                                          "level",
                                          "workload_rate",
                                          "pool_nr",
                                          "promoted",
                                          "rank",
                                          "status",
                                          "team",
                                          "victories_count",
                                          "bouts_count",
                                          "victories_ratio",
                                          "strip",
                                          "time",
                                          NULL);
      filter = new Filter (GetKlassName (),
                           attr_list);

      filter->ShowAttribute ("name");
      filter->ShowAttribute ("first_name");
      filter->ShowAttribute ("club");

      SetFilter (filter);
      filter->Release ();
    }
  }

  // --------------------------------------------------------------------------------
  Round::~Round ()
  {
    _max_score->Release ();
    _matches_per_fencer->Release ();

    Reset ();
  }

  // --------------------------------------------------------------------------------
  void Round::Declare ()
  {
    RegisterStageClass (gettext (_class_name),
                        _xml_class_name,
                        CreateInstance,
                        EDITABLE|REMOVABLE);
  }

  // --------------------------------------------------------------------------------
  void Round::Reset ()
  {
    for (GList *m = _matches; m; m = g_list_next (m))
    {
      Match *match = (Match *) m->data;

      for (guint i = 0; i < 2; i++)
      {
        Player *fencer  = match->GetOpponent (i);
        GList  *matches = (GList *) fencer->GetPtrData (this, "Matches");

        FreeFullGList (Match,
                       matches);

        fencer->RemoveData (this, "Matches");
      }
    }

    g_list_free (_matches);
    _matches = nullptr;
  }

  // --------------------------------------------------------------------------------
  Stage *Round::CreateInstance (StageClass *stage_class)
  {
    return new Round (stage_class);
  }

  // --------------------------------------------------------------------------------
  void Round::Load (xmlNode *xml_node)
  {
    LoadConfiguration (xml_node);
    Garnish ();
    LoadMatches (xml_node);
  }

  // --------------------------------------------------------------------------------
  void Round::LoadMatches (xmlNode *xml_node)
  {
    for (xmlNode *n = xml_node; n != nullptr; n = n->next)
    {
      if (n->type == XML_ELEMENT_NODE)
      {
        if (g_strcmp0 ((char *) n->name, _xml_class_name) == 0)
        {
        }
        else if (g_strcmp0 ((char *) n->name, GetXmlPlayerTag ()) == 0)
        {
          LoadAttendees (n);
        }
        else if (g_strcmp0 ((char *) n->name, "Match") == 0)
        {
          gchar *number = nullptr;

          {
            gchar *attr = (gchar *) xmlGetProp (n, BAD_CAST "ID");

            if (attr)
            {
              number = g_strdup (attr);

              xmlFree (attr);
            }
          }

          if (number)
          {
            for (GList *m = _matches; m; m = g_list_next (m))
            {
              Match       *match = (Match *) m->data;
              const gchar *id    = match->GetName ();

              if (id)
              {
                if (g_strcmp0 (id, number) == 0)
                {
                  for (guint i = 0; i < 2; i++)
                  {
                    LoadMatch (n,
                               match);
                  }
                  break;
                }
              }
            }

            g_free (number);
          }
        }
        else
        {
          return;
        }
      }
      LoadMatches (n->children);
    }
  }

  // --------------------------------------------------------------------------------
  void Round::SaveConfiguration (XmlScheme *xml_scheme)
  {
    Stage::SaveConfiguration (xml_scheme);

    if (_matches_per_fencer)
    {
      _matches_per_fencer->Save (xml_scheme);
    }
  }

  // --------------------------------------------------------------------------------
  void Round::LoadConfiguration (xmlNode *xml_node)
  {
    Stage::LoadConfiguration (xml_node);

    if (_matches_per_fencer)
    {
      _matches_per_fencer->Load (xml_node);
    }
  }

  // --------------------------------------------------------------------------------
  void Round::SaveHeader (XmlScheme *xml_scheme)
  {
    xml_scheme->StartElement (_xml_class_name);

    SaveConfiguration (xml_scheme);

    if (_drop_zones)
    {
      xml_scheme->WriteFormatAttribute ("NbDePoules",
                                        "%d", g_slist_length (_drop_zones));
    }
    xml_scheme->WriteFormatAttribute ("PhaseSuivanteDesQualifies",
                                      "%d", GetId ()+2);
  }

  // --------------------------------------------------------------------------------
  void Round::Save (XmlScheme *xml_scheme)
  {
    SaveHeader (xml_scheme);

    Stage::SaveAttendees (xml_scheme);

    for (GList *m = _matches; m; m = g_list_next (m))
    {
      Match *match = (Match *) m->data;

      match->Save (xml_scheme);
    }

    xml_scheme->EndElement ();
  }

  // --------------------------------------------------------------------------------
  void Round::FillInConfig ()
  {
    Stage::FillInConfig ();

    {
      GtkEntry *w = GTK_ENTRY (_glade->GetWidget ("matches_per_fencer_entry"));

      if (w)
      {
        gchar *text = g_strdup_printf ("%d", _matches_per_fencer->_value);

        gtk_entry_set_text (w,
                            text);
        g_free (text);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Round::ApplyConfig ()
  {
    Stage::ApplyConfig ();

    {
      GtkEntry *w = GTK_ENTRY (_glade->GetWidget ("matches_per_fencer_entry"));

      if (w)
      {
        gchar *str = (gchar *) gtk_entry_get_text (w);

        if (str)
        {
          _matches_per_fencer->_value = atoi (str);
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Round::Garnish ()
  {
    GSList *fencers            = GetShortList ();
    guint   fencer_count       = g_slist_length (fencers);
    guint   matches_per_fencer = MIN (_matches_per_fencer->_value, fencer_count-1);

    for (GSList *f = fencers; f; f = g_slist_next (f))
    {
      Player *fencer  = (Player *) f->data;
      GList  *matches = (GList *) fencer->GetPtrData (this, "Matches");

      for (guint match_count = g_list_length (matches);
                 match_count < matches_per_fencer;
                 match_count++)
      {
        guint32 toss = g_random_int_range (0, fencer_count-1);

        for (GSList *o = g_slist_nth (fencers, toss);
             o != nullptr;
             o = GetNext (fencers, o))
        {
          Player *opponent = (Player *) o->data;

          if ((opponent != fencer) && FencerHasMatch (opponent,
                                                      matches) == FALSE)
          {
            Match *match = new Match (fencer,
                                      opponent,
                                      _max_score);

            matches = g_list_prepend (matches,
                                      match);

            {
              GList *opponent_matches = (GList *) opponent->GetPtrData (this, "Matches");

              opponent_matches = g_list_prepend (opponent_matches,
                                                 match);
              match->Retain ();
              opponent->SetData (this, "Matches",
                                 opponent_matches);
            }

            _matches = g_list_append (_matches,
                                      match);
            match->SetNumber (g_list_length (_matches));
            break;
          }
        }
      }
      fencer->SetData (this, "Matches",
                       matches);
    }
  }

  // --------------------------------------------------------------------------------
  void Round::Display ()
  {
    GooCanvasItem *root = GetRootItem ();

    if (root)
    {
      guint          row   = 0;
      GooCanvasItem *table = goo_canvas_table_new (root,
                                                   NULL);

      for (GList *m = _matches; m; m = g_list_next (m))
      {
        Match *match = (Match *) m->data;

        for (guint i = 0; i < 2; i++)
        {
          Player        *fencer = match->GetOpponent (i);
          GooCanvasItem *image;

          image = GetPlayerImage (table,
                                  "font_desc=\"" BP_FONT "14.0px\"",
                                  fencer,
                                  nullptr,
                                  "name",       "font_weight=\"bold\" foreground=\"darkblue\"",
                                  "first_name", "foreground=\"darkblue\"",
                                  "club",       "style=\"italic\" foreground=\"dimgrey\"",
                                  "league",     "style=\"italic\" foreground=\"dimgrey\"",
                                  "region",     "style=\"italic\" foreground=\"dimgrey\"",
                                  "country",    "style=\"italic\" foreground=\"dimgrey\"",
                                  NULL);

          Canvas::PutInTable (table,
                              image,
                              row, i);
        }
        row++;
      }
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Round::FencerHasMatch (Player *fencer,
                                  GList  *matches)
  {
    for (GList *m = matches; m; m = g_list_next (m))
    {
      Match *match = (Match *) m->data;

      for (guint i = 0; i < 2; i++)
      {
        if (match->GetOpponent (i) == fencer)
        {
          return TRUE;
        }
      }
    }
    return FALSE;
  }

  // --------------------------------------------------------------------------------
  GSList *Round::GetNext (GSList *in,
                          GSList *from)
  {
    GSList *next = g_slist_next (from);

    if (next == nullptr)
    {
      next = in;
    }

    return next;
  }
}
