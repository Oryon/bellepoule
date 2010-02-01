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

#include "module.hpp"
#include "glade.hpp"
#include "schedule.hpp"

class Tournament;

class Contest_c : public Module_c
{
  public:
     Contest_c (gchar *filename);
    ~Contest_c ();

    static void Init ();
    static Contest_c *Create ();

    void AttachTo (GtkNotebook *to);
    void Save ();
    gchar *GetFilename ();
    void AddPlayer (Player_c *player);
    void SetTournament (Tournament *tournament);

  public:
    void on_save_toolbutton_clicked       ();
    void on_properties_toolbutton_clicked ();
    void on_formula_toolbutton_clicked    ();
    void on_backupfile_button_clicked     ();
    void on_contest_close_button_clicked  ();
    void on_calendar_button_clicked       ();
    void on_web_site_button_clicked       ();

  private:
    static const guint _nb_weapon = 3;
    static gchar *weapon_image[_nb_weapon];
    static gchar *weapon_xml_image[_nb_weapon];

    static const guint _nb_gender = 3;
    static gchar *gender_image[_nb_gender];
    static gchar *gender_xml_image[_nb_gender];

    static const guint _nb_category = 8;
    static gchar *category_image[_nb_category];
    static gchar *category_xml_image[_nb_category];

    gchar      *_name;
    gchar      *_organizer;
    gchar      *_web_site;
    guint       _category;
    gchar      *_filename;
    guint       _weapon;
    guint       _gender;
    guint       _day;
    guint       _month;
    guint       _year;
    Schedule_c *_schedule;
    GtkWidget  *_properties_dlg;
    GtkWidget  *_calendar_dlg;
    Tournament *_tournament;
    gboolean    _has_properties;

    GtkWidget *_weapon_combo;
    GtkWidget *_gender_combo;
    GtkWidget *_category_combo;

    Contest_c ();

    void   InitInstance     ();
    void   ReadProperties   ();
    void   SetName          (gchar *name);
    gchar *GetSaveFileName  (gchar *title,
                             gchar *config_key);
    void   Save             (gchar *filename);
    void   FillInProperties ();
    void   FillInDate       (guint day,
                             guint month,
                             guint year);
};

#endif
