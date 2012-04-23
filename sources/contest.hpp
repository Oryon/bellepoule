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

#include "data.hpp"
#include "module.hpp"
#include "glade.hpp"
#include "schedule.hpp"

class Tournament;
class Checkin;

class Contest : public Module
{
  public:
     Contest ();

    ~Contest ();

    static void Init ();

    static Contest *Create ();

    Contest *Duplicate ();

    void Load (const gchar *filename);

    void LatchPlayerList ();

    void AttachTo (GtkNotebook *to);

    void Save ();

    void Publish ();

    gchar *GetFilename ();

    void AddFencer (Player *player,
                    guint   rank);

    void AddReferee (Player *referee);

    void ImportReferees (GSList *imported_list);

    Player *Share (Player *referee);

    Player *GetRefereeFromRef (guint ref);

    State GetState ();

    gchar *GetId ();
    gchar *GetOrganizer ();
    gchar *GetDate ();
    gchar *GetWeapon ();
    gchar  GetWeaponCode ();
    gchar *GetName ();
    gchar *GetDefaultFileName ();
    gchar *GetGender ();
    gchar *GetCategory ();

  public:
    void on_save_toolbutton_clicked       ();
    void on_properties_toolbutton_clicked ();
    void on_formula_toolbutton_clicked    ();
    void on_contest_close_button_clicked  ();
    void on_calendar_button_clicked       ();
    void on_web_site_button_clicked       ();
    void on_ftp_changed                   (GtkComboBox *widget);
    void on_referees_toolbutton_toggled   (GtkToggleToolButton *w);

  private:
    struct Time : public Object
    {
      Time (const gchar *name);

      ~Time ();

      void Load (gchar *attr);
      void Save (xmlTextWriter *xml_writer,
                 const gchar   *attr_name);
      void ReadProperties (Glade *glade);
      void FillInProperties (Glade *glade);
      void Copy (Time *to);
      gboolean IsEqualTo (Time *to);

      gchar *_name;
      guint  _hour;
      guint  _minute;
    };

    static const guint _nb_weapon = 4;
    static const gchar *weapon_image[_nb_weapon];
    static const gchar *weapon_xml_image[_nb_weapon];

    static const guint _nb_gender = 3;
    static const gchar *gender_image[_nb_gender];
    static const gchar *gender_xml_image[_nb_gender];

    static const guint _nb_category = 8;
    static const gchar *category_image[_nb_category];
    static const gchar *category_xml_image[_nb_category];

    static GList *_color_list;

    gchar        *_level;
    gchar        *_id;
    gchar        *_name;
    gchar        *_organizer;
    gchar        *_location;
    gchar        *_web_site;
    guint         _category;
    gchar        *_filename;
    guint         _weapon;
    guint         _gender;
    guint         _day;
    guint         _month;
    guint         _year;
    Time         *_checkin_time;
    Time         *_scratch_time;
    Time         *_start_time;
    Schedule     *_schedule;
    Tournament   *_tournament;
    gboolean      _derived;
    Data         *_color;
    GdkColor     *_gdk_color;
    guint         _save_timeout_id;
    Checkin      *_referees_list;
    gint          _referee_pane_position;
    GHashTable   *_ref_translation_table;
    State         _state;

    GtkWidget   *_weapon_combo;
    GtkWidget   *_gender_combo;
    GtkWidget   *_category_combo;
    GtkNotebook *_notebook;

    void   ReadProperties    ();
    void   DisplayProperties ();
    gchar *GetSaveFileName   (gchar       *title,
                              const gchar *config_key);
    void   Save              (gchar *filename);
    void   FillInProperties  ();
    void   FillInDate        (guint day,
                              guint month,
                              guint year);

    void OnDrawPage (GtkPrintOperation *operation,
                     GtkPrintContext   *context,
                     gint               page_nr);

    void ChooseColor ();

    void MakeDirty ();

    static gboolean OnSaveTimeout (Contest *contest);

    void OnPlugged ();
};

#endif
