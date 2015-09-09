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
#include "application/weapon.hpp"
#include "hall.hpp"

#include "marshaller.hpp"

// --------------------------------------------------------------------------------
Marshaller::Marshaller ()
  : Module ("marshaller.glade")
{
  {
    _hall = new Hall ();

    Plug (_hall,
          _glade->GetWidget ("hall_viewport"));
  }

  _referee_list = NULL;

  {
    GtkNotebook *notebook = GTK_NOTEBOOK (_glade->GetWidget ("referee_notebook"));
    GSList      *current  = Weapon::GetList ();

    while (current)
    {
      Weapon               *weapon   = (Weapon *) current->data;
      People::RefereesList *list     = new People::RefereesList (NULL);
      GtkWidget            *viewport = gtk_viewport_new (NULL, NULL);

      if (_referee_list == NULL)
      {
        _referee_list = list;
      }

      gtk_notebook_append_page (notebook,
                                viewport,
                                gtk_label_new (gettext (weapon->GetImage ())));
      Plug (list,
            viewport);

      current = g_slist_next (current);
    }
  }
}

// --------------------------------------------------------------------------------
Marshaller::~Marshaller ()
{
  _hall->Release ();
}

// --------------------------------------------------------------------------------
void Marshaller::Start ()
{
}

// --------------------------------------------------------------------------------
void Marshaller::OnHttpPost (Net::Message *message)
{
  if (message->GetFitness () > 0)
  {
    if (message->Is ("Competition"))
    {
      _hall->ManageContest (message,
                            GTK_NOTEBOOK (_glade->GetWidget ("batch_notebook")));
    }
    else if (message->Is ("Job"))
    {
      _hall->ManageJob (message);
    }
    else if (message->Is ("Referee"))
    {
      ManageReferee (message);
    }
  }
  else
  {
    if (message->Is ("Competition"))
    {
      _hall->DropContest (message);
    }
    else if (message->Is ("Job"))
    {
      //_hall->ManageJob (message);
    }
  }
}

// --------------------------------------------------------------------------------
void Marshaller::ManageReferee (Net::Message *message)
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
        _referee_list->LoadPlayer (xml_nodeset->nodeTab[0],
                                   "Referee",
                                   NULL);
        _referee_list->OnListChanged ();
      }

      xmlXPathFreeObject  (xml_object);
      xmlXPathFreeContext (xml_context);
    }
    xmlFreeDoc (doc);
  }

  g_free (xml);
}
