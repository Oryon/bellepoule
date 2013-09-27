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

namespace People
{
  const gchar *Barrage::_class_name     = N_ ("Barrage");
  const gchar *Barrage::_xml_class_name = "Barrage";

  // --------------------------------------------------------------------------------
  Barrage::Barrage (StageClass *stage_class)
    : Stage (stage_class),
    PlayersList ("barrage.glade",
                 SORTABLE)
  {
    _ties_count = 0;

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

      filter->ShowAttribute ("promoted");
      filter->ShowAttribute ("name");
      filter->ShowAttribute ("first_name");
      filter->ShowAttribute ("birth_date");
      filter->ShowAttribute ("gender");
      filter->ShowAttribute ("club");
      filter->ShowAttribute ("country");

      SetFilter (filter);
      filter->Release ();
    }

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
                                          "promoted",
                                          "start_rank",
                                          "team",
                                          "victories_ratio",
                                          NULL);
      filter = new Filter (attr_list,
                           this);

      filter->ShowAttribute ("rank");
      filter->ShowAttribute ("status");
#ifdef DEBUG
      filter->ShowAttribute ("previous_stage_rank");
#endif
      filter->ShowAttribute ("name");
      filter->ShowAttribute ("first_name");
      filter->ShowAttribute ("club");

      SetClassificationFilter (filter);
      filter->Release ();
    }

    SetAttributeRight ("promoted",
                       TRUE);
  }

  // --------------------------------------------------------------------------------
  Barrage::~Barrage ()
  {
  }

  // --------------------------------------------------------------------------------
  void Barrage::Declare ()
  {
    RegisterStageClass (gettext (_class_name),
                        _xml_class_name,
                        CreateInstance,
                        EDITABLE);
  }

  // --------------------------------------------------------------------------------
  Stage *Barrage::CreateInstance (StageClass *stage_class)
  {
    return new Barrage (stage_class);
  }

  // --------------------------------------------------------------------------------
  void Barrage::Display ()
  {
    OnAttrListUpdated ();
  }

  // --------------------------------------------------------------------------------
  void Barrage::OnPlugged ()
  {
    Stage *previous_stage = GetPreviousStage ();

    _ties_count = previous_stage->GetQuotaExceedance ();

    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("barrage_classification_toggletoolbutton")),
                                       FALSE);
  }

  // --------------------------------------------------------------------------------
  GSList *Barrage::GetCurrentClassification ()
  {
    Player::AttributeId  rank_attr_id ("rank", GetDataOwner ());
    guint                rank         = 1;
    GSList              *result       = NULL;

    {
      guint   short_list_length = g_slist_length (_attendees->GetShortList ());
      GSList *current           = _attendees->GetShortList ();

      for (guint i = 0; i < short_list_length - _ties_count; i++)
      {
        Player *player = (Player *) current->data;

        player->SetAttributeValue (&rank_attr_id,
                                   rank);
        rank++;
        result = g_slist_append (result,
                                 player);

        current = g_slist_next (current);
      }
    }

    {
      Player::AttributeId promoted_attr_id ("promoted", GetDataOwner ());

      promoted_attr_id.MakeRandomReady (_rand_seed);

      _player_list = g_slist_sort_with_data (_player_list,
                                             (GCompareDataFunc) Player::Compare,
                                             &promoted_attr_id);
      _player_list = g_slist_reverse (_player_list);
    }

    {
      GSList *current = _player_list;

      while (current != NULL)
      {
        Player *player = (Player *) current->data;

        player->SetAttributeValue (&rank_attr_id,
                                   rank);
        rank++;
        result = g_slist_append (result,
                                 player);

        current = g_slist_next (current);
      }
    }

    return result;
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
    LoadConfiguration (xml_node);

#if 0
    for (xmlNode *n = xml_node; n != NULL; n = n->next)
    {
      if (n->type == XML_ELEMENT_NODE)
      {
        if (strcmp ((char *) n->name, _xml_class_name) == 0)
        {
        }
        else if (strcmp ((char *) n->name, GetXmlPlayerTag ()) == 0)
        {
          if (_pool == NULL)
          {
            LoadAttendees (n);
          }
          else
          {
            gchar *attr = (gchar *) xmlGetProp (n, BAD_CAST "REF");

            if (attr)
            {
              Player *player = GetFencerFromRef (atoi (attr));

              if (player)
              {
                _pool->AddFencer (player,
                                  this);
              }
              xmlFree (attr);
            }
          }
        }
        else if (strcmp ((char *) n->name, "Poule") == 0)
        {
          _pool = new Pool (_max_score,
                            1,
                            _contest->GetWeaponCode (),
                            GetXmlPlayerTag (),
                            _rand_seed);
        }
        else if (strcmp ((char *) n->name, "Arbitre") == 0)
        {
        }
        else if (strcmp ((char *) n->name, "Match") == 0)
        {
          _pool->CreateMatchs (NULL);
          _pool->Load (n,
                       _attendees->GetShortList ());
          return;
        }
        else
        {
          return;
        }
      }
      Load (n->children);
    }
#endif
  }

  // --------------------------------------------------------------------------------
  void Barrage::Garnish ()
  {
    Player::AttributeId *promoted_attr_id  = new Player::AttributeId ("promoted", GetDataOwner ());
    guint                short_list_length = g_slist_length (_attendees->GetShortList ());
    GSList              *current;

    current = g_slist_nth (_attendees->GetShortList (),
                           short_list_length - _ties_count);
    while (current)
    {
      Player *player = (Player *) current->data;

      player->SetChangeCbk ("promoted",
                            (Player::OnChange) OnAttrPromotedChanged,
                            this);
      player->SetAttributeValue (promoted_attr_id,
                                 (guint) 1);
      Add (player);

      current = g_slist_next (current);
    }

    promoted_attr_id->Release ();
  }

  // --------------------------------------------------------------------------------
  void Barrage::OnAttrPromotedChanged (Player    *player,
                                       Attribute *attr,
                                       Barrage   *barrage,
                                       guint      step)
  {
    Player::AttributeId status_attr_id ("status", barrage->GetDataOwner ());

    if (attr->GetUIntValue () == 0)
    {
      player->SetAttributeValue (&status_attr_id, "N");
    }
    else
    {
      player->SetAttributeValue (&status_attr_id, "Q");
    }
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_barrage_classification_toggletoolbutton_toggled (GtkToggleToolButton *widget,
                                                                                      Object              *owner)
  {
    Barrage *b = dynamic_cast <Barrage *> (owner);

    b->ToggleClassification (gtk_toggle_tool_button_get_active (widget));
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_barrage_filter_toolbutton_clicked (GtkWidget *widget,
                                                                        Object    *owner)
  {
    Barrage *b = dynamic_cast <Barrage *> (owner);

    b->OnFilterClicked ("barrage_classification_toggletoolbutton");
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_barrage_print_toolbutton_clicked (GtkWidget *widget,
                                                                       Object    *owner)
  {
    Barrage *b = dynamic_cast <Barrage *> (owner);

    b->Print (gettext ("Barrage"));
  }
}
