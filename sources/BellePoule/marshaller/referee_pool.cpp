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
#include "actors/referee.hpp"
#include "referee_pool.hpp"

// --------------------------------------------------------------------------------
RefereePool::RefereePool ()
  : Object ("marshaller.glade")
{
  _referee_list = NULL;
}

// --------------------------------------------------------------------------------
RefereePool::~RefereePool ()
{
  _referee_list->Release ();
}

// --------------------------------------------------------------------------------
void RefereePool::ManageList (People::RefereesList *list)
{
  if (_referee_list == NULL)
  {
    _referee_list = list;
  }
}

// --------------------------------------------------------------------------------
Referee *RefereePool::GetReferee (guint ref)
{
  GSList *current = _referee_list->GetList ();

  while (current)
  {
    Referee *referee = (Referee *) current->data;

    if (referee->GetRef () == ref)
    {
      return referee;
    }

    current = g_slist_next (current);
  }

  return NULL;
}

// --------------------------------------------------------------------------------
Referee *RefereePool::GetRefereeFor (Job *job)
{
  GSList  *list    = _referee_list->GetList ();
  guint    count   = g_slist_length (list);
  Referee *referee = (Referee *) g_slist_nth_data (list, g_random_int_range (0, count));

  return referee;
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
        Player *player = PlayerFactory::CreatePlayer ("Referee");

        player->Load (xml_nodeset->nodeTab[0]);

        if (_referee_list->GetPlayerFromRef (player->GetRef ()) == NULL)
        {
          _referee_list->RegisterPlayer (player,
                                         NULL);
          _referee_list->OnListChanged ();
        }
        else
        {
          player->Release ();
        }
      }

      xmlXPathFreeObject  (xml_object);
      xmlXPathFreeContext (xml_context);
    }
    xmlFreeDoc (doc);
  }

  g_free (xml);
}
