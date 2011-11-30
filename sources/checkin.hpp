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
#include "stage.hpp"
#include "players_list.hpp"
#include "form.hpp"

class Checkin : public virtual Stage, public PlayersList
{
  public:
    static void Init ();

    Checkin (StageClass *stage_class);

    void Add (Player *player);

    void UseInitialRank ();

    void UpdateRanking ();

    void OnListChanged ();

    void ConvertFromBaseToResult ();

  public:
    void on_add_player_button_clicked ();
    void on_remove_player_button_clicked ();

    void OnToggleAllPlayers (gboolean present);

    void OnImport ();
    void OnPrint ();

  private:
    void OnLocked (Reason reason);
    void OnUnLocked ();
    void Wipe ();

  private:
    static const gchar *_class_name;
    static const gchar *_xml_class_name;

    Form       *_form;
    guint       _attendings;
    gboolean    _use_initial_rank;
    GtkWidget  *_print_dialog;
    gboolean    _print_attending;
    gboolean    _print_missing;

    static Stage *CreateInstance (StageClass *stage_class);

    void Load (xmlNode *xml_node);

    void Load (xmlXPathContext *xml_context,
               const gchar     *from_node);

    void Save (xmlTextWriter *xml_writer);

    void ImportFFF (gchar *file);

    void ImportCSV (gchar *file);

    void OnPlugged ();

    void Display ();

    void OnAddPlayer (Player *player);

    gboolean IsOver ();

    void UpdateChecksum ();

    void Monitor (Player *player);

    void RefreshAttendingDisplay ();

    GSList *GetCurrentClassification ();

    void OnPlayerRemoved (Player *player);

    gboolean PlayerIsPrintable (Player *player);

    gchar *ConvertToUtf8 (gchar *what);

    static void OnAttendingChanged (Player    *player,
                                    Attribute *attr,
                                    Checkin   *checkin);

    static gboolean PresentPlayerFilter (Player *player);

    ~Checkin ();
};

#endif
