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

#pragma once

#include "util/module.hpp"
#include "network/ring.hpp"
#include "actors/referees_list.hpp"
#include "hall.hpp"

namespace Marshaller
{
  class SmartPhones;
  class RefereePool;

  class Marshaller :
    public Module,
    public People::RefereesList::Listener,
    public Hall::Listener,
    public Net::Ring::Listener
  {
    public:
      Marshaller ();

      void Start ();

      gboolean OnMessage (Net::Message *message);

      void OnExposeWeapon (const gchar *weapon_code);

      void OnRefereeListCollapse ();

      const gchar *GetSecretKey (const gchar *authentication_scheme);

      void OnMenuDialog (const gchar *dialog);

      void OnShowAccessCode (gboolean with_steps);

      void PrintMealTickets ();

      void PrintPaymentBook ();

    private:
      static const guint NB_TICKET_X_PER_SHEET = 2;
      static const guint NB_TICKET_Y_PER_SHEET = 5;
      static const guint NB_REFEREE_PER_SHEET  = 20;

      Hall        *_hall;
      RefereePool *_referee_pool;
      GtkNotebook *_referee_notebook;
      SmartPhones *_smartphones;
      gboolean     _print_meal_tickets;

      virtual ~Marshaller ();

      void OnEvent (const gchar *event);

      void OnBlur ();

      void OnUnBlur ();

      void OnOpenCheckin (People::RefereesList *referee_list);

      void OnRefereeUpdated (People::RefereesList *referee_list,
                             Player               *referee);

      guint PreparePrint (GtkPrintOperation *operation,
                          GtkPrintContext   *context);

      void DrawPage (GtkPrintOperation *operation,
                     GtkPrintContext   *context,
                     gint               page_nr);

      GList *GetRefereeList ();

      void OnHanshakeResult (Net::Ring::HandshakeResult result);
  };
}
