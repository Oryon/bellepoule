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
      People::PlayersList ("swiss_supervisor.glade", NO_RIGHT)
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

      filter->ShowAttribute ("stage_start_rank");
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

            _matches = g_list_prepend (_matches,
                                       match);
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
    for (GSList *current = GetShortList (); current; current = g_slist_next (current))
    {
      Add ((Player *) current->data);
    }

    OnAttrListUpdated ();
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
