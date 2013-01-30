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

#ifndef form_hpp
#define form_hpp

#include <gtk/gtk.h>

#include "util/object.hpp"
#include "util/filter.hpp"
#include "util/module.hpp"
#include "common/player.hpp"

namespace People
{
  class Form : public Module
  {
    public:
      typedef enum
      {
        UPDATE_PLAYER,
        NEW_PLAYER
      } PlayerEvent;

      typedef void (Module::*PlayerCbk) (Player      *player,
                                         PlayerEvent  event,
                                         guint        page);

      Form (const gchar        *name,
            Module             *client,
            Filter             *filter,
            Player::PlayerType  player_type,
            PlayerCbk           player_cbk);

      void Show (Player *player = NULL);

      void AddPage (const gchar *name,
                    Filter      *filter);

      void Hide ();

      void OnAddButtonClicked ();

      void OnCloseButtonClicked ();

      void Lock ();

      void UnLock ();

    private:
      struct Page
      {
        GtkWidget *_title_vbox;
        GtkWidget *_value_vbox;
        GtkWidget *_check_vbox;
      };

      Module             *_client;
      PlayerCbk           _cbk;
      Player             *_player_to_update;
      gboolean            _locked;
      guint               _page_count;
      Filter             *_filter;
      Player::PlayerType  _player_type;
      Page               *_pages;

      virtual ~Form ();

      void ReadAndWipe (Player *player);

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

#endif
