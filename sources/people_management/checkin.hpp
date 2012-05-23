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

#ifndef checkin_hpp
#define checkin_hpp

#include <gtk/gtk.h>
#include <libxml/xmlwriter.h>

#include "data.hpp"
#include "module.hpp"
#include "attribute.hpp"
#include "players_list.hpp"
#include "form.hpp"

class Checkin : public PlayersList
{
  public:
    Checkin (const gchar *glade,
             const gchar *player_tag);

    virtual void Add (Player *player);

    void LoadList (xmlNode *xml_node);

    void LoadList (xmlXPathContext *xml_context,
                   const gchar     *from_node);

    void SaveList (xmlTextWriter *xml_writer);

  public:
    void on_add_player_button_clicked ();

    void on_remove_player_button_clicked ();

    void OnToggleAllPlayers (gboolean present);

    void OnImport ();

    void OnPrint ();

    void OnListChanged ();

  protected:
    ~Checkin ();

    static gboolean PresentPlayerFilter (Player *player);

    virtual void Monitor (Player *player);

    void CreateForm (Filter *filter);

    virtual void ImportFFF (gchar *file);

    void OnAddPlayerFromForm (Player *player);

  private:
    Form        *_form;
    guint        _attendings;
    GtkWidget   *_print_dialog;
    gboolean     _print_attending;
    gboolean     _print_missing;
    const gchar *_player_tag;
    gchar       *_players_tag;

    virtual void OnLoaded () {};

    virtual void OnPlayerLoaded (Player *player) {};

    void ImportCSV (gchar *file);

    void RefreshAttendingDisplay ();

    void OnPlayerRemoved (Player *player);

    void OnPlugged ();

    gboolean PlayerIsPrintable (Player *player);

    gchar *ConvertToUtf8 (gchar *what);

    Player::PlayerType GetPlayerType ();

    static void OnAttendingChanged (Player    *player,
                                    Attribute *attr,
                                    Object    *owner);
};

#endif
