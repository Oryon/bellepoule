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

#include "round.hpp"

namespace Swiss
{
  const gchar *Round::_class_name     = N_ ("Pool");
  const gchar *Round::_xml_class_name = "RondeSuisse";

  // --------------------------------------------------------------------------------
  Round::Round (StageClass *stage_class)
    : Object ("Swiss::Round"),
      Stage (stage_class),
      Module ("swiss_supervisor.glade")
  {
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
}
