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

#include "common/contest.hpp"
#include "common/classification.hpp"

#include "barrage.hpp"

namespace Pool
{
  const gchar *Barrage::_class_name     = N_ ("Barrage");
  const gchar *Barrage::_xml_class_name = "Barrage";

  // --------------------------------------------------------------------------------
  Barrage::Barrage (StageClass *stage_class)
    : Stage (stage_class),
    Module ("barrage.glade")
  {
    _pool = NULL;

    _max_score = new Data ("ScoreMax",
                           5);

    // Sensitive widgets
    {
      AddSensitiveWidget (_glade->GetWidget ("max_score_entry"));
      AddSensitiveWidget (_glade->GetWidget ("stuff_toolbutton"));

      LockOnClassification (_glade->GetWidget ("stuff_toolbutton"));
    }

    // Filter
    {
      GSList *attr_list;
      Filter *filter;

      AttributeDesc::CreateExcludingList (&attr_list,
#ifndef DEBUG
                                          "ref",
#endif
                                          "HS",
                                          "attending",
                                          "availability",
                                          "exported",
                                          "final_rank",
                                          "global_status",
                                          "indice",
                                          "level",
                                          "participation_rate",
                                          "pool_nr",
                                          "rank",
                                          "start_rank",
                                          "status",
                                          "team",
                                          "victories_ratio",
                                          NULL);
      filter = new Filter (attr_list,
                           this);

      filter->ShowAttribute ("name");
      filter->ShowAttribute ("first_name");

      SetFilter (filter);
      filter->Release ();
    }
  }

  // --------------------------------------------------------------------------------
  Barrage::~Barrage ()
  {
    Object::TryToRelease (_pool);
    Object::TryToRelease (_max_score);
  }

  // --------------------------------------------------------------------------------
  void Barrage::Declare ()
  {
    RegisterStageClass (gettext (_class_name),
                        _xml_class_name,
                        CreateInstance,
                        EDITABLE|REMOVABLE);
  }

  // --------------------------------------------------------------------------------
  Stage *Barrage::CreateInstance (StageClass *stage_class)
  {
    return new Barrage (stage_class);
  }

  // --------------------------------------------------------------------------------
  void Barrage::OnLocked ()
  {
  }

  // --------------------------------------------------------------------------------
  void Barrage::OnUnLocked ()
  {
  }

  // --------------------------------------------------------------------------------
  void Barrage::Display ()
  {
    Plug (_pool,
          _glade->GetWidget ("main_hook"));

    ToggleClassification (FALSE);
  }

  // --------------------------------------------------------------------------------
  void Barrage::Wipe ()
  {
    if (_pool)
    {
      _pool->CleanScores ();
      _pool->Wipe ();

      _pool->UnPlug ();
      _pool = NULL;
    }
  }

  // --------------------------------------------------------------------------------
  GSList *Barrage::GetCurrentClassification ()
  {
    return NULL;
  }

  // --------------------------------------------------------------------------------
  void Barrage::Save (xmlTextWriter *xml_writer)
  {
    xmlTextWriterStartElement (xml_writer,
                               BAD_CAST _xml_class_name);

    SaveConfiguration (xml_writer);
    SaveAttendees     (xml_writer);

    xmlTextWriterEndElement (xml_writer);
  }

  // --------------------------------------------------------------------------------
  void Barrage::Load (xmlNode *xml_node)
  {
  }

  // --------------------------------------------------------------------------------
  void Barrage::Garnish ()
  {
    _pool = new Pool (_max_score,
                      1,
                      _contest->GetWeaponCode (),
                      GetXmlPlayerTag (),
                      _rand_seed);

    _pool->SetDataOwner (this,
                         this,
                         this);
    _pool->SetFilter (_filter);

    if (_attendees)
    {
      GSList *barrage_list = GetBarrageList ();
      GSList *current      = barrage_list;

      while (current)
      {
        Player *fencer = (Player *) current->data;

        _pool->AddFencer (fencer,
                          this);

        current = g_slist_next (current);
      }
      g_slist_free (barrage_list);

      _pool->CreateMatchs (NULL);
    }
  }

  // --------------------------------------------------------------------------------
  void Barrage::OnAttrListUpdated ()
  {
    if (_pool)
    {
      _pool->UnPlug ();
    }
    Display ();
  }

  // --------------------------------------------------------------------------------
  void Barrage::OnFilterClicked ()
  {
    if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("barrage_classification_toggletoolbutton"))))
    {
      Classification *classification = GetClassification ();

      if (classification)
      {
        classification->SelectAttributes ();
      }
    }
    else
    {
      SelectAttributes ();
    }
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_barrage_filter_toolbutton_clicked (GtkWidget *widget,
                                                                        Object    *owner)
  {
    Barrage *t = dynamic_cast <Barrage *> (owner);

    t->OnFilterClicked ();
  }
}
