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

#include <gtk/gtk.h>

#include "util/module.hpp"

class Player;

namespace People
{
  class Form : public Module
  {
    public:
      enum class Event
      {
        UPDATE_PLAYER,
        NEW_PLAYER
      };

      struct Listener
      {
        virtual void OnFormEvent (Player *player,
                                  Event   event) = 0;
      };

    public:
      Form (Module      *client,
            Filter      *filter,
            const gchar *player_class);

      void AddPage (Filter      *filter,
                    const gchar *player_class);

      void Show (GtkWindow *over,
                 Player    *player = nullptr);

      void Hide ();

      void ShowPage (const gchar *page);

      void HidePage (const gchar *page);

      void OnAddButtonClicked ();

      void OnCloseButtonClicked ();

      void Lock ();

      void UnLock ();

      void OnUnmap ();

      void CloseOnAdd ();

      void AddListener (Listener *listener);

    private:
      struct Page
      {
        const gchar *_player_class;
        gboolean     _visible;

        GtkWidget *_title_vbox;
        GtkWidget *_value_vbox;
        GtkWidget *_check_vbox;
        GtkImage  *_flash_code_image;
      };

      gboolean   _close_on_add;
      Module    *_player_owner;
      GList     *_listeners;
      Player    *_player_to_update;
      gboolean   _locked;
      guint      _page_count;
      Page      *_pages;

      ~Form () override;

      void ReadAndWipe (Player *player);

      void ShowTabs ();

      void HideTabs ();

      static void SetSelectorValue (GtkComboBox *combo_box,
                                    const gchar *value);

      static gboolean OnSelectorChanged (GtkEntryCompletion *widget,
                                         GtkTreeModel       *model,
                                         GtkTreeIter        *iter,
                                         GtkComboBox        *combobox);

      static void OnSelectorEntryActivate (GtkEntry    *widget,
                                           GtkComboBox *combobox);

      static void OnSensitiveStateToggled (GtkToggleButton *togglebutton,
                                           GtkWidget       *w);
  };
}
