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

#include "network/message.hpp"
#include "actors/player_factory.hpp"
#include "actors/tally_counter.hpp"
#include "enlisted_referee.hpp"
#include "job.hpp"
#include "slot.hpp"
#include "batch.hpp"
#include "referee_pool.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  RefereePool::RefereePool ()
    : Object ("marshaller.glade")
  {
    _list_by_weapon = NULL;
  }

  // --------------------------------------------------------------------------------
  RefereePool::~RefereePool ()
  {
    g_list_free_full (_list_by_weapon,
                      (GDestroyNotify) Object::TryToRelease);
  }

  // --------------------------------------------------------------------------------
  void RefereePool::ManageList (People::RefereesList *list)
  {
    _list_by_weapon = g_list_prepend (_list_by_weapon,
                                      list);
  }

  // --------------------------------------------------------------------------------
  GList *RefereePool::GetList ()
  {
    return _list_by_weapon;
  }

  // --------------------------------------------------------------------------------
  void RefereePool::RefreshWorkload (const gchar *weapon_code)
  {
    GList *current_weapon = _list_by_weapon;

    while (current_weapon)
    {
      People::RefereesList *referee_list = (People::RefereesList *) current_weapon->data;

      if (g_strcmp0 (referee_list->GetWeaponCode (), weapon_code) == 0)
      {
        gint    all_referee_workload = 0;
        GSList *current_referee;

        current_referee = referee_list->GetList ();
        while (current_referee)
        {
          EnlistedReferee *referee = (EnlistedReferee *) current_referee->data;

          all_referee_workload += referee->GetWorkload ();

          current_referee = g_slist_next (current_referee);
        }

        current_referee = referee_list->GetList ();
        while (current_referee)
        {
          EnlistedReferee *referee = (EnlistedReferee *) current_referee->data;

          referee->SetAllRefereWorkload (all_referee_workload);

          current_referee = g_slist_next (current_referee);
        }

        break;
      }

      current_weapon = g_list_next (current_weapon);
    }
  }

  // --------------------------------------------------------------------------------
  void RefereePool::Spread ()
  {
    g_list_foreach (_list_by_weapon,
                    (GFunc) SpreadWeapon,
                    this);
  }

  // --------------------------------------------------------------------------------
  void RefereePool::SpreadWeapon (People::RefereesList *list,
                                  RefereePool          *rp)
  {
    if (list)
    {
      list->Disclose ("RegisteredReferee");
      list->Spread ();
    }
  }

  // --------------------------------------------------------------------------------
  EnlistedReferee *RefereePool::GetReferee (guint ref)
  {
    GList *current_weapon = _list_by_weapon;

    while (current_weapon)
    {
      People::RefereesList *referee_list = (People::RefereesList *) current_weapon->data;
      GSList               *current      = referee_list->GetList ();

      while (current)
      {
        EnlistedReferee *referee = (EnlistedReferee *) current->data;

        if (referee->GetRef () == ref)
        {
          return referee;
        }

        current = g_slist_next (current);
      }

      current_weapon = g_list_next (current_weapon);
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  gboolean RefereePool::WeaponHasReferees (const gchar *weapon)
  {
    GList *current_weapon = _list_by_weapon;

    while (current_weapon)
    {
      People::RefereesList *referee_list = (People::RefereesList *) current_weapon->data;

      if (g_strcmp0 (referee_list->GetWeaponCode (), weapon) == 0)
      {
        People::TallyCounter *tally_counter = referee_list->GetTallyCounter ();

        return (tally_counter->GetPresentsCount () > 0);
      }

      current_weapon = g_list_next (current_weapon);
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  EnlistedReferee *RefereePool::GetRefereeFor (Job  *job,
                                               Slot *slot)
  {
    GList *current_weapon = _list_by_weapon;
    Batch *batch          = job->GetBatch ();

    while (current_weapon)
    {
      People::RefereesList *referee_list = (People::RefereesList *) current_weapon->data;

      if (g_strcmp0 (referee_list->GetWeaponCode (), batch->GetWeaponCode ()) == 0)
      {
        GSList          *current_referee;
        GSList          *sorted_list;
        EnlistedReferee *available_referee = NULL;

        sorted_list = g_slist_sort (g_slist_copy (referee_list->GetList ()),
                                    (GCompareFunc) EnlistedReferee::CompareWorkload);

        current_referee = sorted_list;
        while (current_referee)
        {
          EnlistedReferee *referee = (EnlistedReferee *) current_referee->data;

          if (referee->IsAvailableFor (slot))
          {
            available_referee = referee;
            break;
          }

          current_referee = g_slist_next (current_referee);
        }

        g_slist_free (sorted_list);
        return available_referee;
      }

      current_weapon = g_list_next (current_weapon);
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  void RefereePool::ManageReferee (Net::Message *message)
  {
    gchar     *xml = message->GetString ("xml");
    xmlDocPtr  doc = xmlParseMemory (xml, strlen (xml));

    if (doc)
    {
      xmlXPathInit ();

      {
        xmlXPathContext *xml_context = xmlXPathNewContext (doc);
        xmlXPathObject  *xml_object;
        xmlNodeSet      *xml_nodeset;

        xml_object = xmlXPathEval (BAD_CAST "/Arbitre", xml_context);
        xml_nodeset = xml_object->nodesetval;

        if (xml_nodeset->nodeNr == 1)
        {
          Player *referee = new EnlistedReferee ();
          Player::AttributeId weapon_attr_id ("weapon");

          referee->Load (xml_nodeset->nodeTab[0]);

          {
            Attribute *weapon_attr    = referee->GetAttribute (&weapon_attr_id);
            GList     *current_weapon = _list_by_weapon;

            while (current_weapon)
            {
              People::RefereesList *referee_list = (People::RefereesList *) current_weapon->data;

              if (g_strcmp0 (referee_list->GetWeaponCode (), weapon_attr->GetStrValue ()) == 0)
              {
                if (referee_list->GetPlayerFromRef (referee->GetRef ()) == NULL)
                {
                  referee_list->RegisterPlayer (referee,
                                                NULL);
                  referee_list->OnListChanged ();
                }
                else
                {
                  referee->Release ();
                }
                break;
              }

              current_weapon = g_list_next (current_weapon);
            }
          }
        }

        xmlXPathFreeObject  (xml_object);
        xmlXPathFreeContext (xml_context);
      }
      xmlFreeDoc (doc);
    }

    g_free (xml);
  }
}
