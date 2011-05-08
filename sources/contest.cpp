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
#include <goocanvas.h>

#ifdef WINDOWS_TEMPORARY_PATCH
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

#include "version.h"
#include "canvas.hpp"
#include "tournament.hpp"
#include "checkin.hpp"

#include "contest.hpp"

const gchar *Contest::weapon_image[_nb_weapon] =
{
  N_ ("Sabre"),
  N_ ("Epée"),
  N_ ("Foil")
};

const gchar *Contest::weapon_xml_image[_nb_weapon] =
{
  "S",
  "E",
  "F"
};

const gchar *Contest::gender_image[_nb_gender] =
{
  N_ ("Male"),
  N_ ("Female"),
  N_ ("Mixed")
};

const gchar *Contest::gender_xml_image[_nb_gender] =
{
  "M",
  "F",
  "FM"
};

const gchar *Contest::category_image[_nb_category] =
{
  N_ ("U8"),
  N_ ("U10"),
  N_ ("U12"),
  N_ ("U14"),
  N_ ("U16"),
  N_ ("U18"),
  N_ ("Senior"),
  N_ ("Veteran")
};

const gchar *Contest::category_xml_image[_nb_category] =
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

GList *Contest::_color_list = NULL;

// --------------------------------------------------------------------------------
Contest::Time::Time (const gchar *name)
{
  _name = g_strdup (name);
  _hour   = 12;
  _minute = 0;
}

// --------------------------------------------------------------------------------
Contest::Time::~Time ()
{
}

// --------------------------------------------------------------------------------
void Contest::Time::Save (xmlTextWriter *xml_writer,
                          const gchar   *attr_name)
{
  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST attr_name,
                                     "%02d:%02d", _hour, _minute);
}

// --------------------------------------------------------------------------------
void Contest::Time::Load (gchar *attr)
{
  if (strlen (attr) > 0)
  {
    gchar **time = g_strsplit (attr,
                               ":",
                               2);

    if (time[0] && time[1])
    {
      _hour   = (guint) atoi (time[0]);
      _minute = (guint) atoi (time[1]);
    }

    g_strfreev (time);
  }
}

// --------------------------------------------------------------------------------
void Contest::Time::ReadProperties (Glade *glade)
{
  gchar *button_name;

  button_name = g_strdup_printf ("%s_hour_spinbutton", _name);
  _hour = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (glade->GetWidget (button_name)));
  g_free (button_name);

  button_name = g_strdup_printf ("%s_minute_spinbutton", _name);
  _minute = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (glade->GetWidget (button_name)));
  g_free (button_name);
}

// --------------------------------------------------------------------------------
void Contest::Time::FillInProperties (Glade *glade)
{
  gchar *button_name;

  button_name = g_strdup_printf ("%s_hour_spinbutton", _name);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (glade->GetWidget (button_name)),
                             _hour);
  g_free (button_name);

  button_name = g_strdup_printf ("%s_minute_spinbutton", _name);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (glade->GetWidget (button_name)),
                             _minute);
  g_free (button_name);
}

// --------------------------------------------------------------------------------
void Contest::Time::Copy (Time *to)
{
  to->_name   = _name;
  to->_hour   = _hour;
  to->_minute = _minute;
}

// --------------------------------------------------------------------------------
Contest::Contest ()
  : Module ("contest.glade")
{
  InitInstance ();

  ChooseColor ();

  _schedule->SetScoreStuffingPolicy (FALSE);
}

// --------------------------------------------------------------------------------
Contest::Contest (gchar *filename)
  : Module ("contest.glade")
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
    gboolean  score_stuffing_policy = FALSE;
    xmlDoc   *doc                   = xmlParseFile (filename);

    if (doc == NULL)
    {
      return;
    }

    xmlXPathInit ();

    // Contest
    if (doc)
    {
      xmlXPathContext *xml_context = xmlXPathNewContext (doc);
      xmlXPathObject  *xml_object;
      xmlNodeSet      *xml_nodeset;

      xml_object  = xmlXPathEval (BAD_CAST "/BaseCompetitionIndividuelle", xml_context);
      xml_nodeset = xml_object->nodesetval;

      if (xml_object->nodesetval->nodeNr)
      {
        gchar *attr;

        if (_color->Load (xml_nodeset->nodeTab[0]))
        {
          _gdk_color = (GdkColor *) g_list_nth_data (_color_list,
                                                     _color->_value);
        }
        else
        {
          ChooseColor ();
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "Championnat");
        if (attr)
        {
          _owner = g_strdup (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "ID");
        if (attr)
        {
          _id = g_strdup (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "TitreLong");
        if (attr)
        {
          _name = g_strdup (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "DateDebut");
        if (attr)
        {
          if (strlen (attr) > 0)
          {
            gchar **date = g_strsplit (attr,
                                       ".",
                                       3);

            if (date[0] && date[1] && date[2])
            {
              _day   = (guint) atoi (date[0]);
              _month = (guint) atoi (date[1]);
              _year  = (guint) atoi (date[2]);
            }

            g_strfreev (date);
          }
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "Appel");
        if (attr)
        {
          _checkin_time->Load (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "Scratch");
        if (attr)
        {
          _scratch_time->Load (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "Début");
        if (attr)
        {
          _start_time->Load (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "Arme");
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

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "Sexe");
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

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "Categorie");
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

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "Organisateur");
        if (attr)
        {
          _organizer = g_strdup (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "URLorganisateur");
        if (attr)
        {
          _web_site = g_strdup (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "score_stuffing");
        if (attr)
        {
          score_stuffing_policy = (gboolean) atoi (attr);
        }
      }
      xmlXPathFreeObject  (xml_object);
      xmlXPathFreeContext (xml_context);
    }

    _schedule->Load (doc);
    if (score_stuffing_policy)
    {
      _schedule->SetScoreStuffingPolicy (score_stuffing_policy);
    }

    xmlFreeDoc (doc);

    {
      GtkRecentManager *manager = gtk_recent_manager_get_default ();

      gtk_recent_manager_add_item (manager, filename);
    }
  }
}

// --------------------------------------------------------------------------------
Contest::~Contest ()
{
  if (_tournament)
  {
    _tournament->OnContestDeleted (this);
  }

  g_free (_owner);
  g_free (_id);
  g_free (_name);
  g_free (_filename);
  g_free (_organizer);
  g_free (_web_site);

  Object::TryToRelease (_checkin_time);
  Object::TryToRelease (_scratch_time);
  Object::TryToRelease (_start_time);

  Object::TryToRelease (_schedule);

  gtk_widget_destroy (_properties_dlg);

  gtk_widget_destroy (_calendar_dlg);

  Object::Dump ();
}

// --------------------------------------------------------------------------------
gchar *Contest::GetFilename ()
{
  return _filename;
}

// --------------------------------------------------------------------------------
void Contest::AddPlayer (Player *player,
                         guint   rank)
{
  if (_schedule)
  {
    Checkin *checkin = dynamic_cast <Checkin *> (_schedule->GetStage (0));

    if (checkin)
    {
      Player::AttributeId  start_rank_attr    ("start_rank");
      Player::AttributeId  previous_rank_attr ("previous_stage_rank", checkin);

      checkin->Add (player);
      player->SetAttributeValue (&start_rank_attr,
                                 rank);
      player->SetAttributeValue (&previous_rank_attr,
                                 rank);
      checkin->UseInitialRank ();
    }
  }
}

// --------------------------------------------------------------------------------
Contest *Contest::Create ()
{
  Contest *contest = new Contest ();

  contest->_name = g_key_file_get_string (_config_file,
                                          "Competiton",
                                          "default_name",
                                          NULL);
  contest->_organizer = g_key_file_get_string (_config_file,
                                               "Competiton",
                                               "default_organizer",
                                               NULL);
  contest->_web_site = g_key_file_get_string (_config_file,
                                              "Competiton",
                                              "default_web_site",
                                              NULL);

  contest->FillInProperties ();
  if (gtk_dialog_run (GTK_DIALOG (contest->_properties_dlg)) == GTK_RESPONSE_ACCEPT)
  {
    contest->_schedule->CreateDefault ();
    contest->ReadProperties ();
    gtk_widget_hide (contest->_properties_dlg);
  }
  else
  {
    Object::TryToRelease (contest);
    contest = NULL;
  }

  return contest;
}

// --------------------------------------------------------------------------------
Contest *Contest::Duplicate ()
{
  Contest *contest = new Contest ();

  contest->_schedule->CreateDefault (TRUE);
  contest->_derived = TRUE;

  if (_owner)
  {
    contest->_owner = g_strdup (_owner);
  }
  if (_id)
  {
    contest->_id = g_strdup (_id);
  }
  contest->_name       = g_strdup (_name);
  contest->_organizer  = g_strdup (_organizer);
  contest->_web_site   = g_strdup (_web_site);
  contest->_category   = _category;
  contest->_weapon     = _weapon;
  contest->_gender     = _gender;
  contest->_day        = _day;
  contest->_month      = _month;
  contest->_year       = _year;
  contest->_tournament = _tournament;

  _checkin_time->Copy (contest->_checkin_time);
  _scratch_time->Copy (contest->_scratch_time);
  _start_time->Copy   (contest->_start_time);

  return contest;
}

// --------------------------------------------------------------------------------
void Contest::LatchPlayerList ()
{
  Checkin *checkin = dynamic_cast <Checkin *> (_schedule->GetStage (0));

  if (checkin)
  {
    Player::AttributeId  rank_attr ("previous_stage_rank", checkin);

    checkin->OnListChanged ();
  }
}

// --------------------------------------------------------------------------------
void Contest::InitInstance ()
{
  Object::Dump ();

  _notebook   = NULL;

  _owner      = NULL;
  _id         = NULL;
  _name       = NULL;
  _filename   = NULL;
  _organizer  = NULL;
  _web_site   = NULL;
  _tournament = NULL;
  _weapon     = 0;
  _category   = 0;
  _gender     = 0;
  _derived    = FALSE;

  _checkin_time = new Time ("checkin");
  _scratch_time = new Time ("scratch");
  _start_time   = new Time ("start");

  _gdk_color = NULL;
  _color     = new Data ("Couleur",
                         (guint) 0);

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
    _schedule = new Schedule (this);

    Plug (_schedule,
          _glade->GetWidget ("schedule_viewport"),
          GTK_TOOLBAR (_glade->GetWidget ("contest_toolbar")));
  }

  {
    GtkWidget *box = _glade->GetWidget ("weapon_box");

    _weapon_combo = gtk_combo_box_new_text ();
    for (guint i = 0; i < _nb_weapon; i ++)
    {
      gtk_combo_box_append_text (GTK_COMBO_BOX (_weapon_combo), gettext (weapon_image[i]));
    }
    gtk_container_add (GTK_CONTAINER (box), _weapon_combo);
    gtk_widget_show (_weapon_combo);
  }

  {
    GtkWidget *box = _glade->GetWidget ("gender_box");

    _gender_combo = gtk_combo_box_new_text ();
    for (guint i = 0; i < _nb_gender; i ++)
    {
      gtk_combo_box_append_text (GTK_COMBO_BOX (_gender_combo), gettext (gender_image[i]));
    }
    gtk_container_add (GTK_CONTAINER (box), _gender_combo);
    gtk_widget_show (_gender_combo);
  }

  {
    GtkWidget *box = _glade->GetWidget ("category_box");

    _category_combo = gtk_combo_box_new_text ();
    for (guint i = 0; i < _nb_category; i ++)
    {
      gtk_combo_box_append_text (GTK_COMBO_BOX (_category_combo), gettext (category_image[i]));
    }
    gtk_container_add (GTK_CONTAINER (box), _category_combo);
    gtk_widget_show (_category_combo);
  }

  // Properties dialog
  {
    GtkWidget *content_area;

    _properties_dlg = gtk_dialog_new_with_buttons (gettext ("Competition properties"),
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

    _calendar_dlg = gtk_dialog_new_with_buttons (gettext ("Date of competition"),
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
void Contest::ChooseColor ()
{
  gint color_to_use;

  color_to_use = g_key_file_get_integer (_config_file,
                                         "Competiton",
                                         "color_to_use",
                                         NULL);
  if (color_to_use >= (gint) (g_list_length (_color_list)-1))
  {
    color_to_use = 0;
  }

  _color->_value = color_to_use;

  color_to_use++;
  if (color_to_use >= (gint) (g_list_length (_color_list)-1))
  {
    color_to_use = 0;
  }
  g_key_file_set_integer (_config_file,
                          "Competiton",
                          "color_to_use",
                          color_to_use);

  _gdk_color = (GdkColor *) g_list_nth_data (_color_list,
                                             _color->_value);
}

// --------------------------------------------------------------------------------
void Contest::Init ()
{
  GdkColor *color;

  color = (GdkColor *) g_malloc (sizeof (GdkColor));
  gdk_color_parse ("#EED680", color); // accent yellow
  _color_list = g_list_append (_color_list, color);

  color = (GdkColor *) g_malloc (sizeof (GdkColor));
  gdk_color_parse ("#E0B6AF", color); // red hilight
  _color_list = g_list_append (_color_list, color);

  color = (GdkColor *) g_malloc (sizeof (GdkColor));
  gdk_color_parse ("#ADA7C8", color); // purple hilight
  _color_list = g_list_append (_color_list, color);

  color = (GdkColor *) g_malloc (sizeof (GdkColor));
  gdk_color_parse ("#83A67F", color); // green medium
  _color_list = g_list_append (_color_list, color);

  color = (GdkColor *) g_malloc (sizeof (GdkColor));
  gdk_color_parse ("#DF421E", color); // accent red
  _color_list = g_list_append (_color_list, color);
}

// --------------------------------------------------------------------------------
void Contest::SetTournament (Tournament *tournament)
{
  _tournament = tournament;
}

// --------------------------------------------------------------------------------
void Contest::FillInDate (guint day,
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
void Contest::FillInProperties ()
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

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("allow_radiobutton")),
                                _schedule->ScoreStuffingIsAllowed ());

  FillInDate (_day,
              _month,
              _year);

  _checkin_time->FillInProperties (_glade);
  _scratch_time->FillInProperties (_glade);
  _start_time->FillInProperties   (_glade);
}

// --------------------------------------------------------------------------------
void Contest::ReadProperties ()
{
  gchar    *str;
  GtkEntry *entry;

  entry = GTK_ENTRY (_glade->GetWidget ("title_entry"));
  str = (gchar *) gtk_entry_get_text (entry);
  g_free (_name);
  _name = g_strdup (str);

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

  _day   = GetUIntData (this, "day");
  _month = GetUIntData (this, "month");
  _year  = GetUIntData (this, "year");

  _checkin_time->ReadProperties (_glade);
  _scratch_time->ReadProperties (_glade);
  _start_time->ReadProperties   (_glade);

  {
    gboolean policy = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("allow_radiobutton")));

    if (_schedule->ScoreStuffingIsAllowed () != policy)
    {
      _schedule->SetScoreStuffingPolicy (policy);
    }
  }

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

  DisplayProperties ();
}

// --------------------------------------------------------------------------------
void Contest::DisplayProperties ()
{
  GtkWidget *w;

  w = _glade->GetWidget ("contest_name_label");
  if (w)
  {
    gtk_label_set_text (GTK_LABEL (w),
                        _name);
  }

  w = _glade->GetWidget ("contest_weapon_label");
  if (w)
  {
    gtk_label_set_text (GTK_LABEL (w),
                        gettext (weapon_image[_weapon]));
  }

  w = _glade->GetWidget ("contest_gender_label");
  if (w)
  {
    gtk_label_set_text (GTK_LABEL (w),
                        gettext (gender_image[_gender]));
  }

  w = _glade->GetWidget ("contest_category_label");
  if (w)
  {
    gtk_label_set_text (GTK_LABEL (w),
                        gettext (category_image[_category]));
  }
}

// --------------------------------------------------------------------------------
void Contest::AttachTo (GtkNotebook *to)
{
  GtkWidget *title = _glade->GetWidget ("notebook_title");

  _notebook = GTK_NOTEBOOK (to);

  gtk_notebook_append_page (_notebook,
                            GetRootWidget (),
                            title);
  g_object_unref (title);

  if (_derived == FALSE)
  {
    gtk_notebook_set_current_page (_notebook,
                                   -1);
  }

  DisplayProperties ();

  if (_gdk_color)
  {
    GtkWidget *tab = _glade->GetWidget ("eventbox");

    gtk_widget_modify_bg (tab,
                          GTK_STATE_NORMAL,
                          _gdk_color);
    gtk_widget_modify_bg (tab,
                          GTK_STATE_ACTIVE,
                          _gdk_color);
  }
}

// --------------------------------------------------------------------------------
void Contest::Save ()
{
  if (_filename == NULL)
  {
    _filename = GetSaveFileName (gettext ("Choose a file..."),
                                 "default_dir_name");
  }

  Save (_filename);
}

// --------------------------------------------------------------------------------
void Contest::Save (gchar *filename)
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

      xmlTextWriterStartDTD (xml_writer,
                             BAD_CAST "BaseCompetitionIndividuelle",
                             NULL,
                             NULL);
      xmlTextWriterEndDTD (xml_writer);

      xmlTextWriterStartComment (xml_writer);
      xmlTextWriterWriteFormatString (xml_writer, "\n");
      xmlTextWriterWriteFormatString (xml_writer, "           By BellePoule (V%s.%s%s)\n", VERSION, VERSION_REVISION, VERSION_MATURITY);
      xmlTextWriterWriteFormatString (xml_writer, "\n");
      xmlTextWriterWriteFormatString (xml_writer, "   http://betton.escrime.free.fr/index.php/bellepoule\n");
      xmlTextWriterEndComment (xml_writer);

      xmlTextWriterStartElement (xml_writer,
                                 BAD_CAST "BaseCompetitionIndividuelle");

      {
        _color->Save (xml_writer);

        if (_owner)
        {
          xmlTextWriterWriteAttribute (xml_writer,
                                       BAD_CAST "Championnat",
                                       BAD_CAST _owner);
        }

        if (_id)
        {
          xmlTextWriterWriteAttribute (xml_writer,
                                       BAD_CAST "ID",
                                       BAD_CAST _id);
        }

        xmlTextWriterWriteFormatAttribute (xml_writer,
                                           BAD_CAST "Annee",
                                           "%d", _year);
        xmlTextWriterWriteAttribute (xml_writer,
                                     BAD_CAST "Arme",
                                     BAD_CAST weapon_xml_image[_weapon]);
        xmlTextWriterWriteAttribute (xml_writer,
                                     BAD_CAST "Sexe",
                                     BAD_CAST gender_xml_image[_gender]);
        xmlTextWriterWriteAttribute (xml_writer,
                                     BAD_CAST "Organisateur",
                                     BAD_CAST _organizer);
        xmlTextWriterWriteAttribute (xml_writer,
                                     BAD_CAST "Categorie",
                                     BAD_CAST category_xml_image[_category]);
        xmlTextWriterWriteFormatAttribute (xml_writer,
                                           BAD_CAST "DateDebut",
                                           "%02d.%02d.%d", _day, _month, _year);
        if (_checkin_time)
        {
          _checkin_time->Save (xml_writer,
                               "Appel");
        }
        if (_scratch_time)
        {
          _scratch_time->Save (xml_writer,
                               "Scratch");
        }
        if (_start_time)
        {
          _start_time->Save   (xml_writer,
                               "Début");
        }
        xmlTextWriterWriteAttribute (xml_writer,
                                     BAD_CAST "TitreLong",
                                     BAD_CAST _name);
        xmlTextWriterWriteAttribute (xml_writer,
                                     BAD_CAST "URLorganisateur",
                                     BAD_CAST _web_site);
        xmlTextWriterWriteFormatAttribute (xml_writer,
                                           BAD_CAST "score_stuffing",
                                           "%d", _schedule->ScoreStuffingIsAllowed ());
      }

      _schedule->Save (xml_writer);

      xmlTextWriterEndElement (xml_writer);
      xmlTextWriterEndDocument (xml_writer);

      xmlFreeTextWriter (xml_writer);

      {
        GtkRecentManager *manager = gtk_recent_manager_get_default ();

        gtk_recent_manager_add_item (manager, filename);
      }
    }
  }
}

// --------------------------------------------------------------------------------
gchar *Contest::GetSaveFileName (gchar       *title,
                                 const gchar *config_key)
{
  GtkWidget *chooser;
  char      *filename = NULL;

  chooser = GTK_WIDGET (gtk_file_chooser_dialog_new (gettext ("Choose a file..."),
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
void Contest::on_web_site_button_clicked ()
{
  GtkWidget *entry = _glade->GetWidget ("web_site_entry");

#ifdef WINDOWS_TEMPORARY_PATCH
  ShellExecuteA (NULL,
                 "open",
                 gtk_entry_get_text (GTK_ENTRY (entry)),
                 NULL,
                 NULL,
                 SW_SHOWNORMAL);
#else
  gtk_show_uri (NULL,
                gtk_entry_get_text (GTK_ENTRY (entry)),
                GDK_CURRENT_TIME,
                NULL);
#endif
}

// --------------------------------------------------------------------------------
void Contest::OnDrawPage (GtkPrintOperation *operation,
                          GtkPrintContext   *context,
                          gint               page_nr)
{
  GooCanvas *canvas = Canvas::CreatePrinterCanvas (context);

  {
    cairo_matrix_t *operation_matrix = (cairo_matrix_t *) g_object_get_data (G_OBJECT (operation),
                                                                             "operation_matrix");

    if (operation_matrix)
    {
      cairo_matrix_t  own_matrix;
      cairo_matrix_t  result_matrix;
      goo_canvas_item_get_transform (goo_canvas_get_root_item (canvas),
                                     &own_matrix);
      cairo_matrix_multiply (&result_matrix,
                             operation_matrix,
                             &own_matrix);
      goo_canvas_item_set_transform (goo_canvas_get_root_item (canvas),
                                     &result_matrix);
    }
  }

  goo_canvas_rect_new (goo_canvas_get_root_item (canvas),
                       0.0, 0.0,
                       100.0, PRINT_HEADER_HEIGHT,
                       "stroke-color", "grey",
                       "fill-color", gdk_color_to_string (_gdk_color),
                       "line-width", 0.3,
                       NULL);

  {
    char *text = g_strdup_printf ("%s - %s - %s", gettext (weapon_image[_weapon]),
                                                   gettext (gender_image[_gender]),
                                                   gettext (category_image[_category]));
    goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                         text,
                         50.5, 3.5,
                         -1.0,
                         GTK_ANCHOR_CENTER,
                         "fill-color", "black",
                         "font", "Sans Bold 4px", NULL);
    goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                         text,
                         50.0, 3.0,
                         -1.0,
                         GTK_ANCHOR_CENTER,
                         "fill-color", "white",
                         "font", "Sans Bold 4px", NULL);
    g_free (text);
  }

  {
    goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                         (const char *) g_object_get_data (G_OBJECT (operation), "job_name"),
                         50.0, 7.5,
                         -1.0,
                         GTK_ANCHOR_CENTER,
                         "fill-color", "black",
                         "font", "Sans Bold 4px", NULL);
  }

  goo_canvas_render (canvas,
                     gtk_print_context_get_cairo_context (context),
                     NULL,
                     1.0);

  gtk_widget_destroy (GTK_WIDGET (canvas));
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_web_site_button_clicked (GtkWidget *widget,
                                                            Object    *owner)
{
  Contest *c = dynamic_cast <Contest *> (owner);

  c->on_web_site_button_clicked ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_save_toolbutton_clicked (GtkWidget *widget,
                                                            Object    *owner)
{
  Contest *c = dynamic_cast <Contest *> (owner);

  c->on_save_toolbutton_clicked ();
}

// --------------------------------------------------------------------------------
void Contest::on_save_toolbutton_clicked ()
{
  Save ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_properties_toolbutton_clicked (GtkWidget *widget,
                                                                  Object    *owner)
{
  Contest *c = dynamic_cast <Contest *> (owner);

  c->on_properties_toolbutton_clicked ();
}

// --------------------------------------------------------------------------------
void Contest::on_properties_toolbutton_clicked ()
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
                                                               Object    *owner)
{
  Contest *c = dynamic_cast <Contest *> (owner);

  c->on_formula_toolbutton_clicked ();
}

// --------------------------------------------------------------------------------
void Contest::on_formula_toolbutton_clicked ()
{
  _schedule->DisplayList ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_backupfile_button_clicked (GtkWidget *widget,
                                                              Object    *owner)
{
  Contest *c = dynamic_cast <Contest *> (owner);

  c->on_backupfile_button_clicked ();
}

// --------------------------------------------------------------------------------
void Contest::on_backupfile_button_clicked ()
{
  gchar *filename = GetSaveFileName (gettext ("Choose a backup file..."),
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
                                                            Object    *owner)
{
  Contest *c = dynamic_cast <Contest *> (owner);

  c->on_calendar_button_clicked ();
}

// --------------------------------------------------------------------------------
void Contest::on_calendar_button_clicked ()
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
                                                                 Object    *owner)
{
  Contest *c = dynamic_cast <Contest *> (owner);

  c->on_contest_close_button_clicked ();
}

// --------------------------------------------------------------------------------
void Contest::on_contest_close_button_clicked ()
{
  GtkWidget *dialog = gtk_message_dialog_new_with_markup (NULL,
                                                          GTK_DIALOG_MODAL,
                                                          GTK_MESSAGE_QUESTION,
                                                          GTK_BUTTONS_OK_CANCEL,
                                                          gettext ("<b><big>Do you really want to close\n"
                                                                   "<big>%s</big> ?</big></b>"), _name);

  gtk_window_set_title (GTK_WINDOW (dialog),
                        gettext ("Close the competition?"));

  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            gettext ("Unsaved data will be lost."));

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
  {
    Release ();
  }
  gtk_widget_destroy (dialog);
}

// --------------------------------------------------------------------------------
gchar *Contest::GetOrganizer ()
{
  return _organizer;
}

// --------------------------------------------------------------------------------
gchar *Contest::GetWeapon ()
{
  return gettext (weapon_image[_weapon]);
}

// --------------------------------------------------------------------------------
gchar *Contest::GetName ()
{
  return _name;
}

// --------------------------------------------------------------------------------
gchar *Contest::GetGender ()
{
  return (char *) gender_xml_image[_gender];
}

// --------------------------------------------------------------------------------
gchar *Contest::GetCategory ()
{
  return gettext (category_image[_category]);
}

// --------------------------------------------------------------------------------
gchar *Contest::GetDate ()
{
  static gchar date[] = "01/01/1970";

  sprintf (date, "%02d/%02d/%02d", _day, _month, _year);
  return date;
}
