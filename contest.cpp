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

#include <string.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include "tournament.hpp"
#include "checkin.hpp"

#include "contest.hpp"

gchar *Contest_c::weapon_image[_nb_weapon] =
{
  "Sabre",
  "Epée",
  "Fleuret"
};

gchar *Contest_c::weapon_xml_image[_nb_weapon] =
{
  "S",
  "E",
  "F"
};

gchar *Contest_c::gender_image[_nb_gender] =
{
  "Masculin",
  "Féminin",
  "Mixte"
};

gchar *Contest_c::gender_xml_image[_nb_gender] =
{
  "M",
  "F",
  "FM"
};

gchar *Contest_c::category_image[_nb_category] =
{
  "Poussin",
  "Pupille",
  "Bejamin",
  "Minime",
  "Cadet",
  "Junior",
  "Senior",
  "Vétéran"
};

gchar *Contest_c::category_xml_image[_nb_category] =
{
  "O",
  "P",
  "B",
  "M",
  "C",
  "J",
  "S",
  "V"
};

// --------------------------------------------------------------------------------
Contest_c::Contest_c ()
  : Module_c ("contest.glade")
{
  InitInstance ();

  _name = g_key_file_get_string (_config_file,
                                 "Competiton",
                                 "default_name",
                                 NULL);
  _organizer = g_key_file_get_string (_config_file,
                                      "Competiton",
                                      "default_organizer",
                                      NULL);
  _web_site = g_key_file_get_string (_config_file,
                                     "Competiton",
                                     "default_web_site",
                                     NULL);

  FillInProperties ();
  if (gtk_dialog_run (GTK_DIALOG (_properties_dlg)) == GTK_RESPONSE_ACCEPT)
  {
    ReadProperties ();
    _schedule->CreateDefault ();
  }
  gtk_widget_hide (_properties_dlg);
}

// --------------------------------------------------------------------------------
Contest_c::Contest_c (gchar *filename)
  : Module_c ("contest.glade")
{
  InitInstance ();

  if (g_path_is_absolute (filename) == FALSE)
  {
    gchar *current_dir = g_get_current_dir ();

    _filename = g_build_filename (current_dir,
                                  filename, NULL);
    g_free (current_dir);
  }
  else
  {
    _filename = g_strdup (filename);
  }

  if (g_file_test (_filename,
                   G_FILE_TEST_IS_REGULAR))
  {
    xmlDoc *doc = xmlParseFile (filename);

    xmlXPathInit ();

    // Contest
    {
      xmlXPathContext *xml_context = xmlXPathNewContext (doc);
      xmlXPathObject  *xml_object;
      xmlNodeSet      *xml_nodeset;

      xml_object  = xmlXPathEval (BAD_CAST "/contest", xml_context);
      xml_nodeset = xml_object->nodesetval;

      if (xml_object->nodesetval->nodeNr)
      {
        gchar *attr;

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "title");
        if (attr)
        {
          SetName (g_strdup (attr));
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "day");
        if (attr)
        {
          _day = (guint) atoi (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "month");
        if (attr)
        {
          _month = (guint) atoi (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "weapon");
        if (attr)
        {
          for (guint i = 0; i < _nb_weapon; i++)
          {
            if (strcmp (attr, weapon_xml_image[i]) == 0)
            {
              _weapon = i;
            }
          }
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "gender");
        if (attr)
        {
          for (guint i = 0; i < _nb_gender; i++)
          {
            if (strcmp (attr, gender_xml_image[i]) == 0)
            {
              _gender = i;
            }
          }
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "category");
        if (attr)
        {
          for (guint i = 0; i < _nb_category; i++)
          {
            if (strcmp (attr, category_xml_image[i]) == 0)
            {
              _category = i;
            }
          }
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "year");
        if (attr)
        {
          _year = (guint) atoi (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "organizer");
        if (attr)
        {
          _organizer = g_strdup (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "web_site");
        if (attr)
        {
          _web_site = g_strdup (attr);
        }

        _has_properties = TRUE;
      }
      xmlXPathFreeObject  (xml_object);
      xmlXPathFreeContext (xml_context);
    }

    _schedule->Load (doc);

    xmlFreeDoc (doc);
  }
  else
  {
    _schedule->CreateDefault ();
  }
}

// --------------------------------------------------------------------------------
Contest_c::~Contest_c ()
{
  if (_tournament)
  {
    _tournament->OnContestDeleted (this);
  }

  g_free (_name);
  g_free (_filename);
  g_free (_organizer);
  g_free (_web_site);

  Object_c::TryToRelease (_schedule);

  gtk_widget_destroy (_properties_dlg);

  gtk_widget_destroy (_calendar_dlg);

  Object_c::Dump ();
}

// --------------------------------------------------------------------------------
gchar *Contest_c::GetFilename ()
{
  return _filename;
}

// --------------------------------------------------------------------------------
void Contest_c::AddPlayer (Player_c *player)
{
  if (_schedule)
  {
    Checkin *checkin = dynamic_cast <Checkin *> (_schedule->GetStage (0));

    if (checkin)
    {
      checkin->Add (player);
    }
  }
}

// --------------------------------------------------------------------------------
Contest_c *Contest_c::Create ()
{
  Contest_c *contest = new Contest_c ();

  if (contest->_has_properties == FALSE)
  {
    Object_c::TryToRelease (contest);
    contest = NULL;
  }

  return contest;
}

// --------------------------------------------------------------------------------
void Contest_c::InitInstance ()
{
  Object_c::Dump ();

  _name           = NULL;
  _filename       = NULL;
  _organizer      = NULL;
  _web_site       = NULL;
  _tournament     = NULL;
  _weapon         = 0;
  _category       = 0;
  _gender         = 0;
  _has_properties = FALSE;

  {
    GTimeVal  current_time;
    GDate    *date         = g_date_new ();

    g_get_current_time (&current_time);
    g_date_set_time_val (date,
                         &current_time);

    _day   = g_date_get_day   (date);
    _month = g_date_get_month (date);
    _year  = g_date_get_year  (date);

    g_date_free (date);
  }

  {
    _schedule = new Schedule_c ();

    Plug (_schedule,
          _glade->GetWidget ("schedule_viewport"),
          GTK_TOOLBAR (_glade->GetWidget ("contest_toolbar")));
  }

  {
    GtkWidget *box = _glade->GetWidget ("weapon_box");

    _weapon_combo = gtk_combo_box_new_text ();
    for (guint i = 0; i < _nb_weapon; i ++)
    {
      gtk_combo_box_append_text (GTK_COMBO_BOX (_weapon_combo), weapon_image[i]);
    }
    gtk_container_add (GTK_CONTAINER (box), _weapon_combo);
    gtk_widget_show (_weapon_combo);
  }

  {
    GtkWidget *box = _glade->GetWidget ("gender_box");

    _gender_combo = gtk_combo_box_new_text ();
    for (guint i = 0; i < _nb_gender; i ++)
    {
      gtk_combo_box_append_text (GTK_COMBO_BOX (_gender_combo), gender_image[i]);
    }
    gtk_container_add (GTK_CONTAINER (box), _gender_combo);
    gtk_widget_show (_gender_combo);
  }

  {
    GtkWidget *box = _glade->GetWidget ("category_box");

    _category_combo = gtk_combo_box_new_text ();
    for (guint i = 0; i < _nb_category; i ++)
    {
      gtk_combo_box_append_text (GTK_COMBO_BOX (_category_combo), category_image[i]);
    }
    gtk_container_add (GTK_CONTAINER (box), _category_combo);
    gtk_widget_show (_category_combo);
  }

  // Properties dialog
  {
    GtkWidget *content_area;

    _properties_dlg = gtk_dialog_new_with_buttons ("Nouvelle compétition",
                                                   NULL,
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_STOCK_OK,
                                                   GTK_RESPONSE_ACCEPT,
                                                   GTK_STOCK_CANCEL,
                                                   GTK_RESPONSE_REJECT,
                                                   NULL);

    content_area = gtk_dialog_get_content_area (GTK_DIALOG (_properties_dlg));

    gtk_widget_reparent (_glade->GetWidget ("properties_dialog-vbox"),
                         content_area);
  }

  // Calendar dialog
  {
    GtkWidget *content_area;

    _calendar_dlg = gtk_dialog_new_with_buttons ("Date de la compétition",
                                                 NULL,
                                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                                 GTK_STOCK_OK,
                                                 GTK_RESPONSE_ACCEPT,
                                                 GTK_STOCK_CANCEL,
                                                 GTK_RESPONSE_REJECT,
                                                 NULL);

    content_area = gtk_dialog_get_content_area (GTK_DIALOG (_calendar_dlg));

    gtk_widget_reparent (_glade->GetWidget ("calendar"),
                         content_area);
  }
}

// --------------------------------------------------------------------------------
void Contest_c::Init ()
{
}

// --------------------------------------------------------------------------------
void Contest_c::SetTournament (Tournament *tournament)
{
  _tournament = tournament;
}

// --------------------------------------------------------------------------------
void Contest_c::FillInDate (guint day,
                            guint month,
                            guint year)
{
  GDate *date = g_date_new ();
  gchar  label[20];

  g_date_set_day   (date, (GDateDay)   day);
  g_date_set_month (date, (GDateMonth) month);
  g_date_set_year  (date, (GDateYear)  year);

  SetData (this, "day",   (void *) day);
  SetData (this, "month", (void *) month);
  SetData (this, "year",  (void *) year);

  g_date_strftime (label,
                   sizeof (label),
                   "%x",
                   date);
  g_date_free (date);

  gtk_button_set_label (GTK_BUTTON (_glade->GetWidget ("calendar_button")),
                        label);

  {
    GtkCalendar *calendar = GTK_CALENDAR (_glade->GetWidget ("calendar"));

    gtk_calendar_select_month (calendar,
                               _month-1,
                               _year);
    gtk_calendar_select_day (calendar,
                             _day);
  }
}

// --------------------------------------------------------------------------------
void Contest_c::FillInProperties ()
{
  gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("title_entry")),
                      _name);

  gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("organizer_entry")),
                      _organizer);

  gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("web_site_entry")),
                      _web_site);

  gtk_combo_box_set_active (GTK_COMBO_BOX (_weapon_combo),
                            _weapon);

  gtk_combo_box_set_active (GTK_COMBO_BOX (_gender_combo),
                            _gender);

  gtk_combo_box_set_active (GTK_COMBO_BOX (_category_combo),
                            _category);

  FillInDate (_day,
              _month,
              _year);
}

// --------------------------------------------------------------------------------
void Contest_c::ReadProperties ()
{
  gchar    *str;
  GtkEntry *entry;

  entry = GTK_ENTRY (_glade->GetWidget ("title_entry"));
  str = (gchar *) gtk_entry_get_text (entry);
  SetName (g_strdup (str));

  entry = GTK_ENTRY (_glade->GetWidget ("organizer_entry"));
  str = (gchar *) gtk_entry_get_text (entry);
  g_free (_organizer);
  _organizer = g_strdup (str);

  entry = GTK_ENTRY (_glade->GetWidget ("web_site_entry"));
  str = (gchar *) gtk_entry_get_text (entry);
  g_free (_web_site);
  _web_site = g_strdup (str);

  _weapon   = gtk_combo_box_get_active (GTK_COMBO_BOX (_weapon_combo));
  _gender   = gtk_combo_box_get_active (GTK_COMBO_BOX (_gender_combo));
  _category = gtk_combo_box_get_active (GTK_COMBO_BOX (_category_combo));

  _day   = (guint) GetData (this, "day");
  _month = (guint) GetData (this, "month");
  _year  = (guint) GetData (this, "year");

  _has_properties = TRUE;

  {
    g_key_file_set_string (_config_file,
                           "Competiton",
                           "default_name",
                           _name);

    g_key_file_set_string (_config_file,
                           "Competiton",
                           "default_organizer",
                           _organizer);

    g_key_file_set_string (_config_file,
                           "Competiton",
                           "default_web_site",
                           _web_site);
  }
}

// --------------------------------------------------------------------------------
void Contest_c::SetName (gchar *name)
{
  g_free (_name);
  _name = name;

  {
    GtkWidget *w = _glade->GetWidget ("contest_label");

    if (w)
    {
      gtk_label_set_text (GTK_LABEL (w),
                          _name);
    }
  }
}

// --------------------------------------------------------------------------------
void Contest_c::AttachTo (GtkNotebook *to)
{
  GtkWidget *title = _glade->GetWidget ("notebook_title");

  gtk_notebook_append_page (to,
                            GetRootWidget (),
                            title);
  g_object_unref (title);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (to),
                                 -1);
}

// --------------------------------------------------------------------------------
void Contest_c::Save ()
{
  Save (_filename);
}

// --------------------------------------------------------------------------------
void Contest_c::Save (gchar *filename)
{
  if (filename)
  {
    xmlTextWriter *xml_writer;

    xml_writer = xmlNewTextWriterFilename (filename, 0);
    if (xml_writer)
    {
      xmlTextWriterSetIndent     (xml_writer,
                                  TRUE);
      xmlTextWriterStartDocument (xml_writer,
                                  NULL,
                                  "UTF-8",
                                  NULL);
      xmlTextWriterStartElement (xml_writer,
                                 BAD_CAST "contest");

      {
        xmlTextWriterWriteAttribute (xml_writer,
                                     BAD_CAST "title",
                                     BAD_CAST _name);
        xmlTextWriterWriteFormatAttribute (xml_writer,
                                           BAD_CAST "day",
                                           "%d", _day);
        xmlTextWriterWriteFormatAttribute (xml_writer,
                                           BAD_CAST "month",
                                           "%d", _month);
        xmlTextWriterWriteFormatAttribute (xml_writer,
                                           BAD_CAST "year",
                                           "%d", _year);
        xmlTextWriterWriteAttribute (xml_writer,
                                     BAD_CAST "weapon",
                                     BAD_CAST weapon_xml_image[_weapon]);
        xmlTextWriterWriteAttribute (xml_writer,
                                     BAD_CAST "gender",
                                     BAD_CAST gender_xml_image[_gender]);
        xmlTextWriterWriteAttribute (xml_writer,
                                     BAD_CAST "category",
                                     BAD_CAST category_xml_image[_category]);
        xmlTextWriterWriteAttribute (xml_writer,
                                     BAD_CAST "organizer",
                                     BAD_CAST _organizer);
        xmlTextWriterWriteAttribute (xml_writer,
                                     BAD_CAST "web_site",
                                     BAD_CAST _web_site);
      }

      _schedule->Save (xml_writer);

      xmlTextWriterEndElement (xml_writer);
      xmlTextWriterEndDocument (xml_writer);

      xmlFreeTextWriter (xml_writer);
    }
  }
}

// --------------------------------------------------------------------------------
gchar *Contest_c::GetSaveFileName (gchar *title,
                                   gchar *config_key)
{
  GtkWidget *chooser;
  char      *filename = NULL;

  chooser = GTK_WIDGET (gtk_file_chooser_dialog_new ("Choisissez un fichier de sauvegarde ...",
                                                     GTK_WINDOW (_glade->GetRootWidget ()),
                                                     GTK_FILE_CHOOSER_ACTION_SAVE,
                                                     GTK_STOCK_CANCEL,
                                                     GTK_RESPONSE_CANCEL,
                                                     GTK_STOCK_SAVE_AS,
                                                     GTK_RESPONSE_ACCEPT,
                                                     NULL));
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (chooser),
                                                  true);

  {
    gchar *last_dirname = g_key_file_get_string (_config_file,
                                                 "Competiton",
                                                 config_key,
                                                 NULL);
    if (last_dirname)
    {
      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser),
                                           last_dirname);

      g_free (last_dirname);
    }
  }

  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
  {
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));

    if (filename)
    {
      {
        gchar *dirname = g_path_get_dirname (filename);

        g_key_file_set_string (_config_file,
                               "Competiton",
                               config_key,
                               dirname);
        g_free (dirname);
      }

      if (strcmp ((const char *) ".cotcot", (const char *) &filename[strlen (filename) - strlen (".cotcot")]) != 0)
      {
        gchar *with_suffix;

        with_suffix = g_strdup_printf ("%s.cotcot", filename);
        g_free (filename);
        filename = with_suffix;
      }
    }
  }

  gtk_widget_destroy (chooser);

  return filename;
}

// --------------------------------------------------------------------------------
void Contest_c::on_web_site_button_clicked ()
{
  GtkWidget *entry = _glade->GetWidget ("web_site_entry");

  ShellExecuteA (NULL,
                 "open",
                 gtk_entry_get_text (GTK_ENTRY (entry)),
                 NULL,
                 NULL,
                 SW_SHOWNORMAL);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_web_site_button_clicked (GtkWidget *widget,
                                                            Object_c  *owner)
{
  Contest_c *c = dynamic_cast <Contest_c *> (owner);

  c->on_web_site_button_clicked ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_save_toolbutton_clicked (GtkWidget *widget,
                                                            Object_c  *owner)
{
  Contest_c *c = dynamic_cast <Contest_c *> (owner);

  c->on_save_toolbutton_clicked ();
}

// --------------------------------------------------------------------------------
void Contest_c::on_save_toolbutton_clicked ()
{
  if (_filename == NULL)
  {
    _filename = GetSaveFileName ("Choisissez un fichier de sauvegarde ...",
                                 "default_dir_name");
  }

  if (_filename)
  {
    Save ();
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_properties_toolbutton_clicked (GtkWidget *widget,
                                                                  Object_c  *owner)
{
  Contest_c *c = dynamic_cast <Contest_c *> (owner);

  c->on_properties_toolbutton_clicked ();
}

// --------------------------------------------------------------------------------
void Contest_c::on_properties_toolbutton_clicked ()
{
  FillInProperties ();
  if (gtk_dialog_run (GTK_DIALOG (_properties_dlg)) == GTK_RESPONSE_ACCEPT)
  {
    ReadProperties ();
  }
  gtk_widget_hide (_properties_dlg);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_formula_toolbutton_clicked (GtkWidget *widget,
                                                               Object_c  *owner)
{
  Contest_c *c = dynamic_cast <Contest_c *> (owner);

  c->on_formula_toolbutton_clicked ();
}

// --------------------------------------------------------------------------------
void Contest_c::on_formula_toolbutton_clicked ()
{
  _schedule->DisplayList ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_backupfile_button_clicked (GtkWidget *widget,
                                                              Object_c  *owner)
{
  Contest_c *c = dynamic_cast <Contest_c *> (owner);

  c->on_backupfile_button_clicked ();
}

// --------------------------------------------------------------------------------
void Contest_c::on_backupfile_button_clicked ()
{
  gchar *filename = GetSaveFileName ("Choisissez un fichier de secours ...",
                                     "default_backup_dir_name");

  if (filename)
  {
    gtk_label_set_text (GTK_LABEL (_glade->GetWidget ("backupfile_label")),
                        filename);
    g_free (filename);
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_calendar_button_clicked (GtkWidget *widget,
                                                            Object_c  *owner)
{
  Contest_c *c = dynamic_cast <Contest_c *> (owner);

  c->on_calendar_button_clicked ();
}

// --------------------------------------------------------------------------------
void Contest_c::on_calendar_button_clicked ()
{
  if (gtk_dialog_run (GTK_DIALOG (_calendar_dlg)) == GTK_RESPONSE_ACCEPT)
  {
    guint year;
    guint month;
    guint day;

    gtk_calendar_get_date (GTK_CALENDAR (_glade->GetWidget ("calendar")),
                           (guint *) &year,
                           (guint *) &month,
                           (guint *) &day);
    month++;
    FillInDate (day,
                month,
                year);
  }
  gtk_widget_hide (_calendar_dlg);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_contest_close_button_clicked (GtkWidget *widget,
                                                                 Object_c  *owner)
{
  Contest_c *c = dynamic_cast <Contest_c *> (owner);

  c->on_contest_close_button_clicked ();
}

// --------------------------------------------------------------------------------
void Contest_c::on_contest_close_button_clicked ()
{
  Release ();
}
