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

#include "form.hpp"
#include "players_list.hpp"

class XmlScheme;

namespace People
{
  class TallyCounter;
  class Source;

  class Checkin : public PlayersList, public Form::Listener
  {
    public:
      struct Listener
      {
        virtual gboolean OnPlayerListRowActivated (Checkin *checkin) = 0;
      };

    public:
      Checkin (const gchar    *glade,
               const gchar    *base_class,
               const gchar    *gathering_class,
               AntiCheatBlock *anti_cheat_block);

      void Add (Player *player) override;

      void AddListener (Listener *listener);

      xmlNode *GetXmlNode (xmlXPathContext *xml_context,
                           const gchar     *from_node,
                           const gchar     *player_class);

      void LoadList (xmlXPathContext *xml_context,
                     const gchar     *from_node,
                     const gchar     *player_class = nullptr);

      Player *LoadPlayer (xmlNode     *xml_node,
                          const gchar *player_class,
                          Player      *owner);

      void RegisterPlayer (Player *player,
                           Player *owner);

      void SaveList (XmlScheme   *xml_scheme,
                     const gchar *player_class);

      void ImportCSV (const gchar *filename);

      void ImportFFF (const gchar *filename);

      virtual void ConvertFromBaseToResult ();

      TallyCounter *GetTallyCounter ();

      Form *CreateForm (Filter      *filter,
                        const gchar *player_class,
                        Module      *player_owner = nullptr);

    public:
      void RaiseForm (GtkWindow *over = nullptr);

      void on_players_list_row_activated (GtkTreePath *path);

      void on_remove_player_button_clicked ();

      void OnToggleAllPlayers (gboolean present);

      void OnImport ();

      void OnPrint ();

    protected:
      Form         *_form;
      TallyCounter *_tally_counter;
      Source       *_source;

      ~Checkin () override;

      static gboolean PresentPlayerFilter (Player      *player,
                                           PlayersList *owner);

      virtual void Monitor (Player *player);

      virtual void SavePlayer (XmlScheme   *xml_scheme,
                               const gchar *player_class,
                               Player      *player);

      void OnFormEvent (Player      *player,
                        Form::Event  event) override;

      void SelectTreeMode () override;

      void SelectFlatMode () override;

      void OnPlayerRemoved (Player *player) override;

      gboolean PlayerIsPrintable (Player *player) override;

      virtual void RefreshAttendingDisplay ();

      void OnListChanged () override;

      void Import (const gchar *filename);

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
                     Player      *owner = nullptr);

      void OnPlugged () override;

      static void OnAttrAttendingChanged (Player    *player,
                                          Attribute *attr,
                                          Object    *owner,
                                          guint      step);

      gchar *GetFileContent (const gchar *filename);

      gchar *GetPrintName () override;

      void GuessPlayerOrganization (Player      *player,
                                    const gchar *organization);
  };
}
