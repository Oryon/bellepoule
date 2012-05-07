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

#include "object.hpp"
#include "filter.hpp"
#include "module.hpp"
#include "player.hpp"

class Form : public Module
{
  public:
    typedef void (Module::*AddPlayerCbk) (Player *player);

    Form (Filter             *filter,
          Module             *client,
          Player::PlayerType  player_type,
          AddPlayerCbk        add_player_cbk);

    void Show ();

    void Hide ();

    void OnAddButtonClicked ();

    void OnCloseButtonClicked ();

  private:
    Filter             *_filter;
    Module             *_client;
    Player::PlayerType  _player_type;
    AddPlayerCbk        _add_player_cbk;

    ~Form ();

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

#endif
