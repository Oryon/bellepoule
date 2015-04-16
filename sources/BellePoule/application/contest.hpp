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

#ifndef contest_hpp
#define contest_hpp

#include <libxml/xmlwriter.h>
#include <gtk/gtk.h>

#include "util/data.hpp"
#include "util/module.hpp"
#include "util/glade.hpp"

#include "schedule.hpp"

namespace People
{
  class Checkin;
}

class Tournament;
class Weapon;
class Partner;

class Contest : public Module
{
  public:
    Contest (gboolean for_duplication = FALSE);

    virtual ~Contest ();

    static void Init ();

    static void Cleanup ();

    static Contest *Create ();

    Contest *Duplicate ();

    void LoadXml (const gchar *filename);

    void LoadFencerFile (const gchar *filename);

    void LatchPlayerList ();

    void AttachTo (GtkNotebook *to);

    void Save ();

    void SaveHeader (xmlTextWriter *xml_writer);

    void DumpToHTML (gchar *filename);

    void Publish ();

    gchar *GetFilename ();

    void AddFencer (Player *player,
                    guint   rank);

    void AddFencer (Player *player);

    Player *GetRefereeFromDndRef (guint ref);

    State GetState ();

    gboolean OnHttpPost (const gchar *command,
                         const gchar **ressource,
                         const gchar *data);

    void SetHallManager (Partner *partner);

    Partner *GetHallManager ();

    gchar    *GetId              ();
    gchar    *GetOrganizer       ();
    gchar    *GetDate            ();
    Weapon   *GetWeapon          ();
    gchar    *GetName            ();
    gchar    *GetDefaultFileName ();
    gchar    *GetGender          ();
    gchar    *GetGenderCode      ();
    gchar    *GetCategory        ();
    gboolean  IsTeamEvent        ();

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

      virtual ~Time ();

      void Load (gchar *attr);
      void Save (xmlTextWriter *xml_writer,
                 const gchar   *attr_name);
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

    static const guint _nb_category = 12;
    static const gchar *category_image[_nb_category];
    static const gchar *category_xml_image[_nb_category];
    static const gchar *category_xml_alias[_nb_category];

    static GList *_color_list;

    gchar           *_level;
    gchar           *_id;
    gchar           *_name;
    gchar           *_organizer;
    gchar           *_location;
    gchar           *_web_site;
    guint            _category;
    gchar           *_filename;
    Weapon          *_weapon;
    guint            _gender;
    guint            _day;
    guint            _month;
    guint            _year;
    gboolean         _team_event;
    Data            *_manual_classification;
    Data            *_default_classification;
    Data            *_minimum_team_size;
    Time            *_checkin_time;
    Time            *_scratch_time;
    Time            *_start_time;
    Schedule        *_schedule;
    Tournament      *_tournament;
    gboolean         _derived;
    GdkColor        *_gdk_color;
    guint            _save_timeout_id;
    People::Checkin *_referees_list;
    State            _state;
    gboolean         _read_only;
    Partner         *_hall_manager;

    GtkWidget   *_properties_dialog;
    GtkWidget   *_weapon_combo;
    GtkWidget   *_gender_combo;
    GtkWidget   *_category_combo;
    GtkNotebook *_notebook;

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
                     gint               page_nr);

    void DrawPage (GtkPrintOperation *operation,
                   GtkPrintContext   *context,
                   gint               page_nr);

    void ChooseColor ();

    void MakeDirty ();

    static gboolean OnSaveTimeout (Contest *contest);

    void OnPlugged ();

    void OnUnPlugged ();

    void UpdateHallManager ();

  private:
    void OnBeginPrint (GtkPrintOperation *operation,
                       GtkPrintContext   *context);
    void OnEndPrint (GtkPrintOperation *operation,
                     GtkPrintContext   *context);
};

#endif
