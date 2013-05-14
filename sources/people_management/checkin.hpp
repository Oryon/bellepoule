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

#include "util/data.hpp"
#include "util/module.hpp"
#include "util/attribute.hpp"

#include "tally_counter.hpp"
#include "players_list.hpp"
#include "form.hpp"

namespace People
{
  class Checkin : public PlayersList
  {
    public:
      Checkin (const gchar *glade,
               const gchar *default_player_class);

      virtual void Add (Player *player);

      void LoadList (xmlXPathContext *xml_context,
                     const gchar     *from_node,
                     const gchar     *player_class = NULL);

      void SaveList (xmlTextWriter *xml_writer,
                     const gchar   *player_class = NULL);

      void ImportCSV (gchar *filename);

      void ImportFFF (gchar *filename);

    public:
      void on_add_player_button_clicked ();

      void on_players_list_row_activated (GtkTreePath *path);

      void on_remove_player_button_clicked ();

      void OnToggleAllPlayers (gboolean present);

      void OnImport ();

      void OnPrint ();

      void OnListChanged ();

    protected:
      Form *_form;

      virtual ~Checkin ();

      static gboolean PresentPlayerFilter (Player *player);

      virtual void Monitor (Player *player);

      void CreateForm (Filter      *filter,
                       const gchar *player_class);

      virtual void OnPlayerEventFromForm (Player            *player,
                                          Form::PlayerEvent  event);

      void ShowTeams ();

      void HideTeams ();

      virtual void OnPlayerRemoved (Player *player);

      virtual void OnAttendingChanged (Player *player,
                                       guint   value);

    private:
      TallyCounter *_tally_counter;
      GtkWidget    *_print_dialog;
      gboolean      _print_attending;
      gboolean      _print_missing;
      const gchar  *_default_player_class;

      virtual void OnPlayerLoaded (Player *player) {};

      void LoadList (xmlNode     *xml_node,
                     const gchar *player_class,
                     const gchar *player_class_xml_tag,
                     const gchar *players_class_xml_tag);

      void RefreshAttendingDisplay ();

      void OnPlugged ();

      gboolean PlayerIsPrintable (Player *player);

      static void OnAttrAttendingChanged (Player    *player,
                                          Attribute *attr,
                                          Object    *owner);

      gchar *GetFileContent (gchar *filename);

      gchar *GetPrintName ();

      void GuessPlayerLeague (Player *player);
  };
}

#endif
