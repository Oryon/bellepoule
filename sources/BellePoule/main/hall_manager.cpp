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

#include "people_management/referees_list.hpp"
#include "application/weapon.hpp"
#include "hall.hpp"
#include "batch.hpp"

#include "hall_manager.hpp"

// --------------------------------------------------------------------------------
HallManager::HallManager ()
  : Module ("hall_manager.glade")
{
  _batch_list = NULL;

  {
    Hall *hall = new Hall ();

    Plug (hall,
          _glade->GetWidget ("hall_viewport"));
  }

  {
    GtkNotebook *notebook = GTK_NOTEBOOK (_glade->GetWidget ("referee_notebook"));
    GSList      *current  = Weapon::GetList ();

    while (current)
    {
      Weapon               *weapon   = (Weapon *) current->data;
      People::RefereesList *list     = new People::RefereesList (NULL);
      GtkWidget            *viewport = gtk_viewport_new (NULL, NULL);

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
  g_list_free_full (_batch_list,
                    (GDestroyNotify) Object::TryToRelease);
}

// --------------------------------------------------------------------------------
void HallManager::Start ()
{
}

// --------------------------------------------------------------------------------
Batch *HallManager::GetBatch (const gchar *id)
{
  GList *current = _batch_list;

  while (current)
  {
    Batch *batch = (Batch *) current->data;

    if (strcmp (batch->GetId (), id) == 0)
    {
      return batch;
    }

    current = g_list_next (current);
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void HallManager::AddContest (const gchar *data)
{
  GKeyFile *key_file = g_key_file_new ();
  GError   *error    = NULL;

  if (g_key_file_load_from_data (key_file,
                                 data,
                                 -1,
                                 G_KEY_FILE_NONE,
                                 &error) == FALSE)
  {
    g_warning ("g_key_file_load_from_data: %s", error->message);
    g_clear_error (&error);
  }
  else
  {
    gchar *id = g_key_file_get_string (key_file,
                                       "Contest",
                                       "id",
                                       NULL);

    if (id)
    {
      GtkWidget *notebook = _glade->GetWidget ("batch_notebook");
      Batch     *batch    = GetBatch (id);

      if (batch == NULL)
      {
        batch = new Batch (id);

        _batch_list = g_list_prepend (_batch_list,
                                      batch);

        batch->AttachTo (GTK_NOTEBOOK (notebook));
      }

      batch->SetProperties (key_file);

      g_free (id);
    }
  }

  g_key_file_free (key_file);
}
