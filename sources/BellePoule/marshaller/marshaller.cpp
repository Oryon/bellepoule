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
#include "util/filter.hpp"

#include "competition.hpp"
#include "affinities.hpp"
#include "enlisted_referee.hpp"
#include "smartphones.hpp"

#include "marshaller.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  Marshaller::Marshaller ()
    : Object ("Marshaller"),
      Module ("marshaller.glade")
  {
    Filter::PreventPersistence ();

    {
      Affinities::Manage ("country", "#fff36c");
      Affinities::Manage ("league",  "#ffb245");
      Affinities::Manage ("club",    "#ffa191");
    }

    _referee_pool = new RefereePool ();

    gtk_window_maximize (GTK_WINDOW (_glade->GetWidget ("root")));

    _referee_notebook = GTK_NOTEBOOK (_glade->GetWidget ("referee_notebook"));

    {
      GList *current = Weapon::GetList ();

      for (guint page = 0 ; current; page++)
      {
        Weapon               *weapon   = (Weapon *) current->data;
        People::RefereesList *list     = new People::RefereesList (this);
        GtkWidget            *viewport = gtk_viewport_new (NULL, NULL);

        list->SetWeapon (weapon);
        list->SetData (this,
                       "page",
                       GUINT_TO_POINTER (page));
        _referee_pool->ManageList (list);

        gtk_notebook_append_page (_referee_notebook,
                                  viewport,
                                  list->GetHeader ());
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

    JobDetails::SetRefereePool (_referee_pool);
  }

  // --------------------------------------------------------------------------------
  Marshaller::~Marshaller ()
  {
    _hall->Release         ();
    _referee_pool->Release ();
    _smartphones->Release  ();
  }

  // --------------------------------------------------------------------------------
  void Marshaller::Start ()
  {
    _smartphones = new SmartPhones (_glade);
  }

  // --------------------------------------------------------------------------------
  gboolean Marshaller::OnHttpPost (Net::Message *message)
  {
    if (message->GetFitness () > 0)
    {
      if (message->Is ("Competition"))
      {
        _hall->ManageCompetition (message,
                                  GTK_NOTEBOOK (_glade->GetWidget ("batch_notebook")));
        return TRUE;
      }
      else if (message->Is ("Batch"))
      {
        _hall->ManageBatch (message);
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
      else if (message->Is ("RefereeHandshake"))
      {
        _referee_pool->ManageHandShake (message);
        return TRUE;
      }
    }
    else
    {
      if (message->Is ("Competition"))
      {
        _hall->DeleteCompetition (message);
        return TRUE;
      }
      else if (message->Is ("Batch"))
      {
        _hall->DeleteBatch (message);
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
  gchar *Marshaller::GetSecretKey (const gchar *authentication_scheme)
  {
    WifiCode *wifi_code = NULL;

    if (authentication_scheme)
    {
      gchar **tokens = g_strsplit_set (authentication_scheme,
                                       "/",
                                       0);

      if (tokens)
      {
        if (tokens[0] && tokens[1] && tokens[2])
        {
          guint            ref     = atoi (tokens[2]);
          EnlistedReferee *referee = _referee_pool->GetReferee (ref);

          if (referee)
          {
            wifi_code = (WifiCode *) referee->GetFlashCode ();
          }
          g_strfreev (tokens);
        }
      }
    }

    if (wifi_code)
    {
      return wifi_code->GetKey ();
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  void Marshaller::OnOpenCheckin (People::RefereesList *referee_list)
  {
    {
      GtkPaned      *paned      = GTK_PANED (_glade->GetWidget ("hpaned"));
      GtkAllocation  allocation;

      gtk_widget_get_allocation (GetRootWidget (),
                                 &allocation);

      gtk_paned_set_position (paned, allocation.width);
    }

    gtk_notebook_set_current_page (_referee_notebook,
                                   referee_list->GetIntData (this,
                                                             "page"));

    _referee_pool->ExpandAll ();

    gtk_notebook_set_show_tabs (_referee_notebook,
                                FALSE);
    gtk_widget_set_visible (_glade->GetWidget ("hall_viewport"),
                            FALSE);
    gtk_widget_set_visible (_glade->GetWidget ("batch_vbox"),
                            FALSE);
    gtk_widget_set_visible (_glade->GetWidget ("collapse"),
                            TRUE);
  }

  // --------------------------------------------------------------------------------
  void Marshaller::OnRefereeListCollapse ()
  {
    {
      GtkPaned *paned = GTK_PANED (_glade->GetWidget ("hpaned"));

      gtk_paned_set_position (paned, 0);
    }

    _referee_pool->CollapseAll ();

    gtk_notebook_set_show_tabs (_referee_notebook,
                                TRUE);
    gtk_widget_set_visible (_glade->GetWidget ("hall_viewport"),
                            TRUE);
    gtk_widget_set_visible (_glade->GetWidget ("batch_vbox"),
                            TRUE);
    gtk_widget_set_visible (_glade->GetWidget ("collapse"),
                            FALSE);
  }

  // --------------------------------------------------------------------------------
  void Marshaller::OnBlur ()
  {
    gtk_widget_set_sensitive (_glade->GetWidget ("referee_vbox"),
                              FALSE);
    gtk_widget_set_sensitive (_glade->GetWidget ("batch_vbox"),
                              FALSE);
  }

  // --------------------------------------------------------------------------------
  void Marshaller::OnUnBlur ()
  {
    gtk_widget_set_sensitive (_glade->GetWidget ("referee_vbox"),
                              TRUE);
    gtk_widget_set_sensitive (_glade->GetWidget ("batch_vbox"),
                              TRUE);
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
        gtk_notebook_set_current_page (_referee_notebook,
                                       i);
        break;
      }

      current = g_list_next (current);
    }
  }

  // --------------------------------------------------------------------------------
  void Marshaller::OnMenuDialog (const gchar *dialog)
  {
    GtkWidget *w = _glade->GetWidget (dialog);

    RunDialog (GTK_DIALOG (w));
    gtk_widget_hide (w);
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_batch_notebook_switch_page (GtkNotebook *notebook,
                                                                 gpointer     page,
                                                                 guint        page_num,
                                                                 Object      *owner)
  {
    Marshaller  *m           = dynamic_cast <Marshaller *> (owner);
    Competition *competition = (Competition *) g_object_get_data (G_OBJECT (page), "competition");

    m->OnExposeWeapon (competition->GetWeaponCode ());
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_SmartPoule_button_clicked (GtkButton *widget,
                                                                gpointer   user_data)
  {
#ifdef WINDOWS_TEMPORARY_PATCH
    ShellExecute (NULL,
                  "open",
                  "https://play.google.com/store/apps/details?id=betton.escrime.smartpoule",
                  NULL,
                  NULL,
                  SW_SHOWNORMAL);
#else
    gtk_show_uri (NULL,
                  "https://play.google.com/store/apps/details?id=betton.escrime.smartpoule",
                  GDK_CURRENT_TIME,
                  NULL);
#endif
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_collapse_clicked (GtkToolButton *widget,
                                                       Object        *owner)
  {
    Marshaller *m = dynamic_cast <Marshaller *> (owner);

    m->OnRefereeListCollapse ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_network_config_menuitem_activate (GtkWidget *w,
                                                                       Object    *owner)
  {
    Marshaller *m = dynamic_cast <Marshaller *> (owner);

    m->OnMenuDialog ("network_dialog");
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_scanner_menuitem_activate (GtkWidget *w,
                                                                Object    *owner)
  {
    Marshaller *m = dynamic_cast <Marshaller *> (owner);

    m->OnMenuDialog ("scanner_dialog");
  }
}
