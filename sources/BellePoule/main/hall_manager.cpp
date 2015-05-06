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
void HallManager::OnHttpPost (const gchar *data)
{
  gchar **lines = g_strsplit_set (data,
                                  "\n",
                                  0);
  if (lines[0])
  {
    const gchar *body = data;

    body = strstr (body, "\n"); if (body) body++;

    if (strcmp (lines[0], "/Competition") == 0)
    {
      _hall->ManageContest (body,
                            GTK_NOTEBOOK (_glade->GetWidget ("batch_notebook")));
    }
    else if (strcmp (lines[0], "/Job") == 0)
    {
      _hall->ManageJob (body);
    }
    else if (strcmp (lines[0], "/Referee") == 0)
    {
      ManageReferee (body);
    }
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
