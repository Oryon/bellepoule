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
#include <libxml/parser.h>
#include <libxml/xpath.h>

#include "util/module.hpp"
#include "network/advertiser.hpp"

namespace People
{
  class RefereesList;
}

namespace AskFred
{
  namespace Reader
  {
    class Event;
  }
}

namespace Net
{
  class Message;
}

class Data;
class Tournament;
class Weapon;
class Category;
class Schedule;
class XmlScheme;
class Player;

class Contest : public Module
{
  public:
    Contest (GList    *advertisers,
             gboolean  for_duplication = FALSE);

    ~Contest () override;

    static void Init ();

    static void Cleanup ();

    void AskForSettings ();

    Contest *Duplicate (const gchar *label = nullptr);

    void LoadAskFred (AskFred::Reader::Event *askfred,
                      const gchar            *dirname);

    void LoadXml (const gchar *filename);

    void LoadFencerFile (const gchar *filename);

    void AttachTo (GtkNotebook *to);

    void Save ();

    void SaveHeader (XmlScheme *xml_scheme);

    void DumpToHTML (gchar *filename);

    void DumpToFRD (gchar *filename);

    void Publish ();

    gchar *GetFilename ();

    void AddFencer (Player *player,
                    guint   rank);

    void AddFencer (Player *player);

    void ManageReferee (Net::Message *message);

    Player *GetRefereeFromRef (guint ref);

    State GetState () override;

    gboolean OnMessage (Net::Message *message) override;

    void TweetFeeder (Net::Advertiser::Feeder *feeder);

    void MakeDirty () override;

    guint        GetNetID           ();
    gchar       *GetOrganizer       ();
    gchar       *GetDate            ();
    Weapon      *GetWeapon          ();
    gchar       *GetName            ();
    gchar       *GetDefaultFileName ();
    gchar       *GetGender          ();
    gchar       *GetGenderCode      ();
    const gchar *GetCategory        ();
    gboolean     IsTeamEvent        ();
    gboolean     HasAskFredEntry    ();

  public:
    void ReadTeamProperty                 ();
    void on_save_toolbutton_clicked       ();
    void on_properties_toolbutton_clicked ();
    void on_contest_close_button_clicked  ();
    void on_calendar_button_clicked       ();
    void on_web_site_button_clicked       ();

  private:
    struct Time : public Object
    {
      Time (const gchar *name);

      ~Time () override;

      void Load (gchar *attr);
      void Save (XmlScheme   *xml_scheme,
                 const gchar *attr_name);
      void ReadProperties (Glade *glade);
      void HideProperties (Glade *glade);
      void FillInProperties (Glade *glade);
      void Copy (Time *to);
      gboolean IsEqualTo (Time *to);

      gchar *_name;
      guint  _hour;
      guint  _minute;
    };

    static const guint _nb_gender = 3;
    static const gchar *gender_image[_nb_gender];
    static const gchar *gender_xml_image[_nb_gender];
    static const gchar *gender_xml_alias[_nb_gender];

    static const guint _nb_level = 7;
    static const gchar *level_image[_nb_level];
    static const gchar *level_xml_image[_nb_level];

    static GList *_color_list;

    gchar                *_authority;
    gchar                *_fie_id;
    gchar                *_name;
    gchar                *_organizer;
    gchar                *_source;
    gchar                *_location;
    gchar                *_label;
    gchar                *_web_site;
    Category             *_category;
    guint                 _level;
    gchar                *_filename;
    Weapon               *_weapon;
    guint                 _gender;
    guint                 _day;
    guint                 _month;
    guint                 _year;
    gboolean              _team_event;
    Data                 *_manual_classification;
    Data                 *_minimum_team_size;
    Data                 *_default_classification;
    Time                 *_checkin_time;
    Time                 *_scratch_time;
    Time                 *_start_time;
    Schedule             *_schedule;
    Tournament           *_tournament;
    gboolean              _derived;
    GdkColor             *_gdk_color;
    guint                 _save_timeout_id;
    People::RefereesList *_referees_list;
    State                 _state;
    gboolean              _read_only;
    GList                *_advertisers;

    GtkWidget   *_properties_dialog;
    GtkWidget   *_weapon_combo;
    GtkWidget   *_gender_combo;
    GtkWidget   *_level_combo;

    void   ReadProperties       ();
    void   DisplayProperties    ();
    void   LoadXmlDoc           (xmlDoc *doc);
    void   GetUnknownAttributes (const gchar     *contest_keyword,
                                 xmlXPathContext *xml_context);
    void   LoadHeader           (xmlXPathContext *context);
    gchar *GetSaveFileName      (GtkWidget   *chooser,
                                 const gchar *config_key,
                                 gint        *choice = NULL);
    void   Save                 (gchar *filename);
    void   FillInProperties     ();
    void   FillInDate           (guint day,
                                 guint month,
                                 guint year);

    void OnDrawPage (GtkPrintOperation *operation,
                     GtkPrintContext   *context,
                     gint               page_nr) override;

    void DrawPage (GtkPrintOperation *operation,
                   GtkPrintContext   *context,
                   gint               page_nr) override;

    void ChooseColor ();

    static gboolean OnSaveTimeout (Contest *contest);

    void OnPlugged () override;

    void FeedParcel (Net::Message *parcel) override;

    void AddFileToRecentManager (const gchar *filename);

    void SetFilename (gchar *filename);

  private:
    void OnBeginPrint (GtkPrintOperation *operation,
                       GtkPrintContext   *context) override;
    void OnEndPrint (GtkPrintOperation *operation,
                     GtkPrintContext   *context) override;
};
