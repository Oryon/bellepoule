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
#include "actors/checkin.hpp"
#include "actors/player_factory.hpp"
#include "actors/tally_counter.hpp"
#include "actors/referees_list.hpp"
#include "enlisted_referee.hpp"
#include "job.hpp"
#include "slot.hpp"
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
    FreeFullGList (People::RefereesList, _list_by_weapon);
  }

  // --------------------------------------------------------------------------------
  void RefereePool::ManageList (People::RefereesList *list)
  {
    _list_by_weapon = g_list_prepend (_list_by_weapon,
                                      list);

    // Popup menu
    {
      GtkUIManager *ui_manager = list->GetUiManager ();

      {
        GError *error = NULL;
        static const gchar xml[] =
          "<ui>\n"
          "  <popup name='PopupMenu'>\n"
          "    <separator/>\n"
          "    <menuitem action='JobListAction'/>\n"
          "  </popup>\n"
          "</ui>";

        if (gtk_ui_manager_add_ui_from_string (ui_manager,
                                               xml,
                                               -1,
                                               &error) == FALSE)
        {
          g_message ("building menus failed: %s", error->message);
          g_error_free (error);
          error = NULL;
        }
      }

      // Actions
      {
        GtkActionGroup *action_group = gtk_action_group_new ("RefereesList::JobActions");
        static GtkActionEntry entries[] =
        {
          {"JobListAction", GTK_STOCK_JUSTIFY_FILL, gettext ("Display jobs"), NULL, NULL, G_CALLBACK (OnDisplayJobs)}
        };

        gtk_action_group_add_actions (action_group,
                                      entries,
                                      G_N_ELEMENTS (entries),
                                      list);
        gtk_ui_manager_insert_action_group (ui_manager,
                                            action_group,
                                            0);

        g_object_unref (G_OBJECT (action_group));
      }

      list->AddListener (this);
    }
  }

  // --------------------------------------------------------------------------------
  void RefereePool::ExpandAll ()
  {
    GList *current = _list_by_weapon;

    while (current)
    {
      People::RefereesList *list = (People::RefereesList *) current->data;

      list->Expand ();

      current = g_list_next (current);
    }
  }

  // --------------------------------------------------------------------------------
  void RefereePool::CollapseAll ()
  {
    GList *current = _list_by_weapon;

    while (current)
    {
      People::RefereesList *list = (People::RefereesList *) current->data;

      list->Collapse ();

      current = g_list_next (current);
    }
  }

  // --------------------------------------------------------------------------------
  People::RefereesList *RefereePool::GetListOf (const gchar *weapon)
  {
    GList *current_weapon = _list_by_weapon;

    while (current_weapon)
    {
      People::RefereesList *referee_list = (People::RefereesList *) current_weapon->data;

      if (g_strcmp0 (referee_list->GetWeaponCode (), weapon) == 0)
      {
        return referee_list;
      }

      current_weapon = g_list_next (current_weapon);
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  void RefereePool::RefreshWorkload (const gchar *weapon_code)
  {
    People::RefereesList *referee_list         = GetListOf (weapon_code);
    gint                  all_referee_workload = 0;
    GList                *current_referee;

    current_referee = referee_list->GetList ();
    while (current_referee)
    {
      EnlistedReferee *referee = (EnlistedReferee *) current_referee->data;

      all_referee_workload += referee->GetWorkload ();

      current_referee = g_list_next (current_referee);
    }

    current_referee = referee_list->GetList ();
    while (current_referee)
    {
      EnlistedReferee *referee = (EnlistedReferee *) current_referee->data;

      referee->SetAllRefereWorkload (all_referee_workload);

      current_referee = g_list_next (current_referee);
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
      GList                *current      = referee_list->GetList ();

      while (current)
      {
        EnlistedReferee *referee = (EnlistedReferee *) current->data;

        if (referee->GetRef () == ref)
        {
          return referee;
        }

        current = g_list_next (current);
      }

      current_weapon = g_list_next (current_weapon);
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  gboolean RefereePool::WeaponHasReferees (const gchar *weapon)
  {
    People::RefereesList *referee_list  = GetListOf (weapon);
    People::TallyCounter *tally_counter = referee_list->GetTallyCounter ();

    return (tally_counter->GetPresentsCount () > 0);
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
            Attribute            *weapon_attr  = referee->GetAttribute (&weapon_attr_id);
            People::RefereesList *referee_list = GetListOf (weapon_attr->GetStrValue ());

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
          }
        }

        xmlXPathFreeObject  (xml_object);
        xmlXPathFreeContext (xml_context);
      }
      xmlFreeDoc (doc);
    }

    g_free (xml);
  }

  // --------------------------------------------------------------------------------
  gboolean RefereePool::OnPlayerListRowActivated (People::Checkin *checkin)
  {
    People::RefereesList *rl = dynamic_cast <People::RefereesList *> (checkin);

    if (rl)
    {
      if (rl->IsCollapsed ())
      {
        OnDisplayJobs (NULL,
                       rl);
        return TRUE;
      }
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void RefereePool::OnDisplayJobs (GtkWidget            *w,
                                   People::RefereesList *referee_list)
  {
    GList *selection = referee_list->GetSelectedPlayers ();

    if (selection)
    {
      EnlistedReferee *referee = (EnlistedReferee *) selection->data;

      referee->DisplayJobs ();

      g_list_free (selection);
    }
  }
}
