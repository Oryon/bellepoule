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

#include "hall_manager.hpp"

// --------------------------------------------------------------------------------
HallManager::HallManager ()
  : Module ("hall_manager.glade")
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
HallManager::~HallManager ()
{
  _hall->Release ();
}

// --------------------------------------------------------------------------------
void HallManager::Start ()
{
}

// --------------------------------------------------------------------------------
void HallManager::OnHttpPost (Net::Message *message)
{
  if (message->Is ("/Competition"))
  {
    if (message->GetInteger ("Ring::Validity") == 1)
    {
      _hall->ManageContest (message,
                            GTK_NOTEBOOK (_glade->GetWidget ("batch_notebook")));
    }
    else
    {
      _hall->DropContest (message);
    }
  }
  else if (message->Is ("/Job"))
  {
    // _hall->ManageJob (body);
  }
  else if (message->Is ("/Referee"))
  {
    // ManageReferee (body);
  }
}

// --------------------------------------------------------------------------------
void HallManager::ManageReferee (const gchar *data)
{
  xmlDocPtr doc = xmlParseMemory (data, strlen (data));

  if (doc)
  {
    xmlXPathContext *xml_context = xmlXPathNewContext (doc);

    _referee_list->LoadList (xml_context,
                             "");

    xmlXPathFreeContext (xml_context);
    xmlFreeDoc (doc);
  }
}
