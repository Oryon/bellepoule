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
#include <libxml/xpath.h>

#include "util/data.hpp"
#include "util/module.hpp"
#include "util/attribute.hpp"
#include "form.hpp"

#include "players_list.hpp"

namespace People
{
  class TallyCounter;

  class Checkin : public PlayersList, public Form::Listener
  {
    public:
      struct Listener
      {
        virtual gboolean OnPlayerListRowActivated (Checkin *checkin) = 0;
      };

    public:
      Checkin (const gchar *glade,
               const gchar *base_class,
               const gchar *gathering_class);

      virtual void Add (Player *player);

      void AddListener (Listener *listener);

      void LoadList (xmlXPathContext *xml_context,
                     const gchar     *from_node,
                     const gchar     *player_class = NULL);

      Player *LoadPlayer (xmlNode     *xml_node,
                          const gchar *player_class,
                          Player      *owner);

      void RegisterPlayer (Player *player,
                           Player *owner);

      void SaveList (xmlTextWriter *xml_writer,
                     const gchar   *player_class = NULL);

      void ImportCSV (gchar *filename);

      void ImportFFF (gchar *filename);

      virtual void ConvertFromBaseToResult ();

      TallyCounter *GetTallyCounter ();

    public:
      void on_add_player_button_clicked ();

      void on_players_list_row_activated (GtkTreePath *path);

      void on_remove_player_button_clicked ();

      void OnToggleAllPlayers (gboolean present);

      void OnImport ();

      void OnPrint ();

      virtual void OnListChanged ();

    protected:
      Form         *_form;
      TallyCounter *_tally_counter;

      virtual ~Checkin ();

      static gboolean PresentPlayerFilter (Player      *player,
                                           PlayersList *owner);

      virtual void Monitor (Player *player);

      void CreateForm (Filter      *filter,
                       const gchar *player_class);

      virtual void SavePlayer (xmlTextWriter *xml_writer,
                               const gchar   *player_class,
                               Player        *player);

      virtual void OnFormEvent (Player          *player,
                                Form::FormEvent  event);

      void SelectTreeMode ();

      void SelectFlatMode ();

      virtual void OnPlayerRemoved (Player *player);

      virtual gboolean PlayerIsPrintable (Player *player);

      virtual void RefreshAttendingDisplay ();


    private:
      Listener     *_listener;
      gboolean      _print_attending;
      gboolean      _print_missing;
      const gchar  *_base_class;
      const gchar  *_gathering_class;

      virtual void OnPlayerLoaded (Player *player,
                                   Player *owner) {};

      void LoadList (xmlNode     *xml_node,
                     const gchar *player_class,
                     const gchar *player_class_xml_tag,
                     const gchar *players_class_xml_tag,
                     Player      *owner = NULL);

      void OnPlugged ();

      static void OnAttrAttendingChanged (Player    *player,
                                          Attribute *attr,
                                          Object    *owner,
                                          guint      step);

      gchar *GetFileContent (gchar *filename);

      gchar *GetPrintName ();

      void GuessPlayerLeague (Player *player);
  };
}
