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

#include "util/dnd_config.hpp"
#include "util/attribute.hpp"
#include "util/attribute_desc.hpp"
#include "util/filter.hpp"
#include "util/player.hpp"
#include "network/message.hpp"
#include "message_uploader.hpp"
#include "network/ring.hpp"
#include "network/partner.hpp"
#include "actors/checkin.hpp"
#include "actors/player_factory.hpp"
#include "actors/tally_counter.hpp"
#include "actors/referees_list.hpp"
#include "enlisted_referee.hpp"
#include "job.hpp"
#include "slot.hpp"
#include "affinities.hpp"
#include "referee_pool.hpp"
#include "timeline.hpp"
#include "piste.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  RefereePool::RefereePool ()
    : Object ("marshaller.glade")
  {
    _list_by_weapon = nullptr;

    {
      _free_color = g_new (GdkColor, 1);

      gdk_color_parse ("darkgreen",
                       _free_color);
    }
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
        GError *error = nullptr;
        static const gchar xml[] =
          "<ui>\n"
          "  <popup name='PopupMenu'>\n"
          "    <separator/>\n"
          "    <menuitem action='JobListAction'/>\n"
          "    <menuitem action='ActivateAction'/>\n"
          "  </popup>\n"
          "</ui>";

        if (gtk_ui_manager_add_ui_from_string (ui_manager,
                                               xml,
                                               -1,
                                               &error) == FALSE)
        {
          g_message ("building menus failed: %s", error->message);
          g_error_free (error);
          error = nullptr;
        }
      }

      // Popup
      {
        static GtkActionEntry entries[] =
        {
          {"JobListAction",  GTK_STOCK_JUSTIFY_FILL, gettext ("Display jobs"), nullptr, nullptr, G_CALLBACK (OnDisplayJobs)}
        };

        list->AddPopupEntries ("RefereesList::JobActions",
                               G_N_ELEMENTS (entries),
                               entries);
      }

      // Popup
      {
        static GtkActionEntry entries[] =
        {
          {"ActivateAction", GTK_STOCK_MEDIA_PAUSE, gettext ("Available/Not available"), nullptr, nullptr, G_CALLBACK (OnToggleAvailability)}
        };

        list->AddPopupEntries ("RefereesList::ActivateAction",
                               G_N_ELEMENTS (entries),
                               entries);
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
    for (GList *current = _list_by_weapon; current; current = g_list_next (current))
    {
      People::RefereesList *referee_list = (People::RefereesList *) current->data;

      if (g_ascii_strcasecmp (referee_list->GetWeaponCode (), weapon) == 0)
      {
        return referee_list;
      }
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  void RefereePool::SetDndPeerListener (DndConfig::Listener *listener)
  {
    GList *current_weapon = _list_by_weapon;

    while (current_weapon)
    {
      People::RefereesList *referee_list = (People::RefereesList *) current_weapon->data;
      DndConfig            *dnd_config   = referee_list->GetDndConfig ();

      dnd_config->SetPeerListener (listener);

      current_weapon = g_list_next (current_weapon);
    }
  }

  // --------------------------------------------------------------------------------
  void RefereePool::RefreshWorkload (const gchar *weapon_code)
  {
    People::RefereesList *referee_list         = GetListOf (weapon_code);
    gint                  all_referee_workload = 0;

    for (GList *current = referee_list->GetList (); current; current = g_list_next (current))
    {
      EnlistedReferee *referee = (EnlistedReferee *) current->data;

      all_referee_workload += referee->GetWorkload ();
    }

    for (GList *current = referee_list->GetList (); current; current = g_list_next (current))
    {
      EnlistedReferee *referee = (EnlistedReferee *) current->data;

      referee->SetAllRefereWorkload (all_referee_workload);
    }
  }

  // --------------------------------------------------------------------------------
  void RefereePool::RefreshAvailability (Timeline *timeline,
                                         GList    *pistes)
  {
    _timeline   = timeline;
    _piste_list = pistes;

    g_list_foreach (_list_by_weapon,
                    (GFunc) RefreshWeaponAvailability,
                    this);
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
      list->Disclose ("BellePoule2D::Referee");
      list->Spread ();
    }
  }

  // --------------------------------------------------------------------------------
  void RefereePool::RefreshWeaponAvailability (People::RefereesList *list,
                                               RefereePool          *rp)
  {
    GList     *current_referee;
    GDateTime *cursor          = rp->_timeline->RetreiveCursorTime (FALSE);

    current_referee = list->GetList ();
    while (current_referee)
    {
      EnlistedReferee *referee = (EnlistedReferee *) current_referee->data;

      referee->RemoveData (nullptr,
                           "RefereesList::CellColor");

      current_referee = g_list_next (current_referee);
    }

    current_referee = list->GetList ();
    while (current_referee)
    {
      EnlistedReferee *referee       = (EnlistedReferee *) current_referee->data;
      GList           *current_piste = rp->_piste_list;

      while (current_piste)
      {
        Piste *piste = (Piste *) current_piste->data;
        Slot  *slot  = piste->GetSlotAt (cursor);

        if (slot)
        {
          GList *slot_referees = slot->GetRefereeList ();

          while (slot_referees)
          {
            if (referee == (EnlistedReferee *) slot_referees->data)
            {
              if (referee->IsAvailableFor (slot,
                                           slot->GetDuration ()) == FALSE)
              {
                GList *jobs = slot->GetJobList ();

                if (jobs)
                {
                  Job *job = (Job *) jobs->data;

                  referee->SetData (nullptr,
                                    "RefereesList::CellColor",
                                    gdk_color_copy (job->GetGdkColor ()),
                                    (GDestroyNotify) gdk_color_free);
                }
              }
              break;
            }
            slot_referees = g_list_next (slot_referees);
          }
        }

        current_piste = g_list_next (current_piste);
      }
      current_referee = g_list_next (current_referee);
    }

    g_date_time_unref (cursor);

    list->OnAttrListUpdated ();
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

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  gboolean RefereePool::WeaponHasReferees (const gchar *weapon)
  {
    People::RefereesList *referee_list  = GetListOf (weapon);
    People::TallyCounter *tally_counter = referee_list->GetTallyCounter ();

    return (tally_counter->GetPresentsCount () > 0);
  }

  // --------------------------------------------------------------------------------
  void RefereePool::ManageHandShake (Net::Message *message)
  {
    EnlistedReferee *referee = GetReferee (message->GetInteger ("referee_id"));

    if (referee)
    {
      gchar *referee_address = message->GetString ("address");

      if (referee_address)
      {
        Player::AttributeId connection_attr_id ("connection");
        Player::AttributeId ip_attr_id         ("IP");

        referee->SetAttributeValue (&connection_attr_id,
                                    "OK");
        referee->SetAttributeValue (&ip_attr_id,
                                    referee_address);

        referee->Spread ();
      }
      g_free (referee_address);
    }
  }

  // --------------------------------------------------------------------------------
  void RefereePool::UpdateAffinities (Player *referee)
  {
    referee->SetData (nullptr,
                      "affinities",
                      new Affinities (referee),
                      (GDestroyNotify) Affinities::Destroy);
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
          Player *new_referee;
          guint   ref;
          gchar  *weapon;

          {
            Player *submitted_referee = new EnlistedReferee ();

            submitted_referee->Load (xml_nodeset->nodeTab[0]);

            {
              Player::AttributeId  weapon_attr_id ("weapon");
              Attribute           *weapon_attr  = submitted_referee->GetAttribute (&weapon_attr_id);

              weapon = weapon_attr->GetStrValue ();
            }

            ref = submitted_referee->GetRef ();

            new_referee = GetReferee (ref);
            if (new_referee)
            {
              new_referee->Retain ();
              submitted_referee->Release ();
            }
            else
            {
              new_referee = submitted_referee;
            }
          }

          for (gchar *w = weapon; *w != '\0'; w++)
          {
            gchar                 current_weapon[2] = {w[0], 0};
            People::RefereesList *referee_list = GetListOf (current_weapon);

            if (referee_list && (referee_list->GetPlayerFromRef (ref) == nullptr))
            {
              UpdateAffinities (new_referee);

              new_referee->Retain ();
              referee_list->RegisterPlayer (new_referee,
                                            nullptr);
            }
          }

          new_referee->Release ();
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
        OnDisplayJobs (nullptr,
                       rl);
        return TRUE;
      }
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void RefereePool::OnDisplayJobs (GtkWidget            *widget,
                                   People::RefereesList *referee_list)
  {
    GList *selection = referee_list->RetreiveSelectedPlayers ();

    if (selection)
    {
      EnlistedReferee *referee = (EnlistedReferee *) selection->data;

      referee->DisplayJobs ();

      g_list_free (selection);
    }
  }

  // --------------------------------------------------------------------------------
  void RefereePool::OnToggleAvailability (GtkWidget            *widget,
                                          People::RefereesList *referee_list)
  {
    GList *selection = referee_list->RetreiveSelectedPlayers ();

    if (selection)
    {
      EnlistedReferee     *referee = (EnlistedReferee *) selection->data;
      Player::AttributeId  weapon_attr_id ("weapon");
      Attribute           *weapon_attr = referee->GetAttribute (&weapon_attr_id);
      gchar               *weapons     = weapon_attr->GetStrValue ();

      for (gchar *w = weapons; *w != '\0'; w++)
      {
        if (g_ascii_toupper (w[0]) == referee_list->GetWeaponCode ()[0])
        {
          if (g_ascii_isupper (w[0]))
          {
            w[0] = g_ascii_tolower (w[0]);
          }
          else
          {
            w[0] = g_ascii_toupper (w[0]);
          }

          referee->SetAttributeValue (&weapon_attr_id,
                                      weapons);
          referee->Spread ();
          break;
        }
      }
    }
  }
}
