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
#include "network/ring.hpp"
#include "application/weapon.hpp"

#include "actors/referees_list.hpp"

#include "marshaller.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  Marshaller::Marshaller ()
    : Object ("Marshaller"),
      Module ("marshaller.glade")
  {
    _referee_pool = new RefereePool ();

    gtk_window_maximize (GTK_WINDOW (_glade->GetWidget ("root")));

    {
      GtkNotebook *notebook = GTK_NOTEBOOK (_glade->GetWidget ("referee_notebook"));
      GList       *current  = Weapon::GetList ();

      while (current)
      {
        Weapon               *weapon   = (Weapon *) current->data;
        People::RefereesList *list     = new People::RefereesList ();
        GtkWidget            *viewport = gtk_viewport_new (NULL, NULL);

        list->SetWeapon (weapon);
        _referee_pool->ManageList (list);

        gtk_notebook_append_page (notebook,
                                  viewport,
                                  gtk_label_new (gettext (weapon->GetImage ())));
        Plug (list,
              viewport);

        current = g_list_next (current);
      }
    }

    {
      _hall = new Hall (_referee_pool,
                        this);

      Plug (_hall,
            _glade->GetWidget ("hall_viewport"));
    }
  }

  // --------------------------------------------------------------------------------
  Marshaller::~Marshaller ()
  {
    _hall->Release         ();
    _referee_pool->Release ();
  }

  // --------------------------------------------------------------------------------
  void Marshaller::Start ()
  {
  }

  // --------------------------------------------------------------------------------
  gboolean Marshaller::OnHttpPost (Net::Message *message)
  {
    if (message->GetFitness () > 0)
    {
      if (message->Is ("Competition"))
      {
        _hall->ManageContest (message,
                              GTK_NOTEBOOK (_glade->GetWidget ("batch_notebook")));
        return TRUE;
      }
      else if (message->Is ("Job"))
      {
        _hall->ManageJob (message);
        return TRUE;
      }
      else if (message->Is ("Referee"))
      {
        _referee_pool->ManageReferee (message);
        return TRUE;
      }
      else if (message->Is ("Fencer"))
      {
        _hall->ManageFencer (message);
        return TRUE;
      }
    }
    else
    {
      if (message->Is ("Competition"))
      {
        _hall->DeleteContest (message);
        return TRUE;
      }
      else if (message->Is ("Job"))
      {
        _hall->DeleteJob (message);
        return TRUE;
      }
      else if (message->Is ("Fencer"))
      {
        _hall->DeleteFencer (message);
        return TRUE;
      }
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void Marshaller::OnRefereeListExpand ()
  {
    {
      GtkPaned      *paned      = GTK_PANED (_glade->GetWidget ("hpaned"));
      GtkAllocation  allocation;

      gtk_widget_get_allocation (GetRootWidget (),
                                 &allocation);

      gtk_paned_set_position (paned, allocation.width);
    }

    _referee_pool->ExpandAll ();
  }

  // --------------------------------------------------------------------------------
  void Marshaller::OnRefereeListCollapse ()
  {
    {
      GtkPaned *paned = GTK_PANED (_glade->GetWidget ("hpaned"));

      gtk_paned_set_position (paned, 0);
    }

    _referee_pool->CollapseAll ();
  }

  // --------------------------------------------------------------------------------
  void Marshaller::OnExposeWeapon (const gchar *weapon_code)
  {
    GList *current = Weapon::GetList ();

    for (guint i = 0; current != NULL; i++)
    {
      Weapon *weapon = (Weapon *) current->data;

      if (g_strcmp0 (weapon->GetXmlImage (), weapon_code) == 0)
      {
        gtk_notebook_set_current_page (GTK_NOTEBOOK (_glade->GetWidget ("referee_notebook")),
                                       i);
        break;
      }

      current = g_list_next (current);
    }
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_batch_notebook_switch_page (GtkNotebook *notebook,
                                                                 gpointer     page,
                                                                 guint        page_num,
                                                                 Object      *owner)
  {
    Marshaller *m     = dynamic_cast <Marshaller *> (owner);
    Batch      *batch = (Batch *) g_object_get_data (G_OBJECT (page), "batch");

    m->OnExposeWeapon (batch->GetWeaponCode ());
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_edit_referees_toggled (GtkToggleButton *togglebutton,
                                                            Object          *owner)
  {
    Marshaller *m = dynamic_cast <Marshaller *> (owner);

    if (gtk_toggle_button_get_active (togglebutton))
    {
      m->OnRefereeListExpand ();
    }
    else
    {
      m->OnRefereeListCollapse ();
    }
  }
}
