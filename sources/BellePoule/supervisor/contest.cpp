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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>
#include <goocanvas.h>

#ifdef WINDOWS_TEMPORARY_PATCH
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

#include "util/global.hpp"
#include "util/canvas.hpp"
#include "actors/checkin.hpp"
#include "actors/referees_list.hpp"
#include "rounds/checkin/checkin_supervisor.hpp"
#include "ecosystem.hpp"

#include "application/version.h"
#include "application/weapon.hpp"

#include "tournament.hpp"
#include "contest.hpp"

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

const gchar *Contest::gender_xml_alias[_nb_gender] =
{
  "M",
  "D",
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
  N_ ("Veteran"),
  N_ ("Veteran 1"),
  N_ ("Veteran 2"),
  N_ ("Veteran 3"),
  N_ ("Veteran 4")
};

const gchar *Contest::category_xml_image[_nb_category] =
{
  "PO",
  "PUP",
  "BEN",
  "M",
  "C",
  "J",
  "S",
  "VET",
  "V1",
  "V2",
  "V3",
  "V4"
};

const gchar *Contest::category_xml_alias[_nb_category] =
{
  "O",
  "P",
  "B",
  "M",
  "C",
  "J",
  "S",
  "V",
  "V",
  "V",
  "V",
  "V"
};

GList *Contest::_color_list = NULL;

// --------------------------------------------------------------------------------
Contest::Time::Time (const gchar *name)
  : Object ("Contest::Time")
{
  _name = g_strdup (name);
  _hour   = 12;
  _minute = 0;
}

// --------------------------------------------------------------------------------
Contest::Time::~Time ()
{
  g_free (_name);
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
    gchar **time = g_strsplit_set (attr,
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
void Contest::Time::HideProperties (Glade *glade)
{
  gchar *button_name;

  button_name = g_strdup_printf ("%s_hour_spinbutton", _name);
  gtk_widget_set_sensitive (glade->GetWidget (button_name), FALSE);
  g_free (button_name);

  button_name = g_strdup_printf ("%s_minute_spinbutton", _name);
  gtk_widget_set_sensitive (glade->GetWidget (button_name), FALSE);
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
  to->_name   = g_strdup (_name);
  to->_hour   = _hour;
  to->_minute = _minute;
}

// --------------------------------------------------------------------------------
gboolean Contest::Time::IsEqualTo (Time *to)
{
  return (   (to->_hour   == _hour)
          && (to->_minute == _minute));
}

// --------------------------------------------------------------------------------
Contest::Contest (gboolean for_duplication )
  : Module ("contest.glade")
{
  _save_timeout_id = 0;

  {
    time_t current_time;

    time (&current_time);
    _id = g_strdup_printf ("%x", (GTime) current_time);
  }

  _read_only  = FALSE;
  _level      = NULL;
  _filename   = NULL;
  _tournament = NULL;
  _weapon     = Weapon::GetDefault ();
  _category   = 0;
  _gender     = 0;
  _team_event = FALSE;
  _derived    = FALSE;

  Disclose ("Competition");

  _name = g_key_file_get_string (Global::_user_config->_key_file,
                                 "Competiton",
                                 "default_name",
                                 NULL);
  _organizer = g_key_file_get_string (Global::_user_config->_key_file,
                                      "Competiton",
                                      "default_organizer",
                                      NULL);
  _web_site = g_key_file_get_string (Global::_user_config->_key_file,
                                     "Competiton",
                                     "default_web_site",
                                     NULL);
  _location = g_key_file_get_string (Global::_user_config->_key_file,
                                     "Competiton",
                                     "default_location",
                                     NULL);

  _manual_classification  = new Data ("ClassementManuel",       (guint) (for_duplication == TRUE));
  _minimum_team_size      = new Data ("TailleMinimaleEquipe",   3);
  _default_classification = new Data ("ClassementDefautEquipe", 9999);

  _state = OPERATIONAL;

  _checkin_time = new Time ("checkin");
  _scratch_time = new Time ("scratch");
  _start_time   = new Time ("start");

  ChooseColor ();

  _properties_dialog = _glade->GetWidget ("properties_dialog");

  SetData (NULL,
           "SensitiveWidgetForCheckinStage",
           _glade->GetWidget ("team_vbox"));

  _referees_list = new People::RefereesList (this);

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
    _schedule = new Schedule (this,
                              _minimum_team_size,
                              _manual_classification,
                              _default_classification);

    Plug (_schedule,
          _glade->GetWidget ("schedule_viewport"),
          GTK_TOOLBAR (_glade->GetWidget ("contest_toolbar")),
          GTK_CONTAINER (_glade->GetWidget ("formula_alignment")));
  }

  {
    GtkWidget *box     = _glade->GetWidget ("weapon_box");
    GSList    *current = Weapon::GetList ();

    _weapon_combo = gtk_combo_box_text_new ();
    while (current)
    {
      Weapon *weapon = (Weapon *) current->data;

      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (_weapon_combo),
                                      weapon->GetImage ());
      current = g_slist_next (current);
    }
    gtk_container_add (GTK_CONTAINER (box), _weapon_combo);
    gtk_widget_show (_weapon_combo);
  }

  {
    GtkWidget *box = _glade->GetWidget ("gender_box");

    _gender_combo = gtk_combo_box_text_new ();
    for (guint i = 0; i < _nb_gender; i ++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (_gender_combo), gettext (gender_image[i]));
    }
    gtk_container_add (GTK_CONTAINER (box), _gender_combo);
    gtk_widget_show (_gender_combo);
  }

  {
    GtkWidget *box = _glade->GetWidget ("category_box");

    _category_combo = gtk_combo_box_text_new ();
    for (guint i = 0; i < _nb_category; i ++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (_category_combo), gettext (category_image[i]));
    }
    gtk_container_add (GTK_CONTAINER (box), _category_combo);
    gtk_widget_show (_category_combo);
  }
}

// --------------------------------------------------------------------------------
void Contest::GetUnknownAttributes (const gchar     *contest_keyword,
                                    xmlXPathContext *xml_context)
{
  gchar          *xml_object_path = g_strdup_printf ("/%s/Tireurs/Tireur", contest_keyword);
  xmlXPathObject *xml_object      = xmlXPathEval (BAD_CAST xml_object_path, xml_context);

  g_free (xml_object_path);

  if (xml_object)
  {
    xmlNodeSet *xml_nodeset = xml_object->nodesetval;

    if (xml_object->nodesetval->nodeNr)
    {
      xmlAttr *current_attr = xml_nodeset->nodeTab[0]->properties;

      while (current_attr)
      {
        if (AttributeDesc::GetDescFromXmlName ((const gchar *) current_attr->name) == NULL)
        {
          AttributeDesc::Declare (G_TYPE_STRING,
                                  (const gchar *) current_attr->name,
                                  (const gchar *) current_attr->name,
                                  (gchar *) current_attr->name);
        }

        current_attr = current_attr->next;
      }
    }

    xmlXPathFreeObject  (xml_object);
  }
}

// --------------------------------------------------------------------------------
void Contest::LoadXml (const gchar *filename)
{
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

    if (doc)
    {
      LoadXmlDoc (doc);

      {
        gchar *uri = g_filename_to_uri (filename,
                                        NULL,
                                        NULL);

        gtk_recent_manager_add_item (gtk_recent_manager_get_default (),
                                     uri);
        g_free (uri);
      }

      if (g_str_has_suffix (_filename,
                            ".cotcot") == FALSE)
      {
        g_free (_filename);
        _filename = NULL;
      }
      else
      {
        GtkWidget *save_toolbutton = _glade->GetWidget ("save_toolbutton");

        gtk_widget_set_tooltip_text (save_toolbutton,
                                     _filename);
        gtk_widget_set_sensitive (save_toolbutton,
                                  FALSE);
      }

      if (_save_timeout_id > 0)
      {
        g_source_remove (_save_timeout_id);
      }

      xmlFreeDoc (doc);
    }
  }

  if ((_filename == NULL) && (_read_only == FALSE))
  {
    GtkWidget *chooser = GTK_WIDGET (gtk_file_chooser_dialog_new (gettext ("Choose a file..."),
                                                                  NULL,
                                                                  GTK_FILE_CHOOSER_ACTION_SAVE,
                                                                  GTK_STOCK_CANCEL,  GTK_RESPONSE_CANCEL,
                                                                  GTK_STOCK_SAVE_AS, GTK_RESPONSE_ACCEPT,
                                                                  NULL));

    GetSaveFileName (chooser,
                     "default_dir_name");
    gtk_widget_destroy (chooser);
  }

  Save ();
}

// --------------------------------------------------------------------------------
void Contest::LoadFencerFile (const gchar *filename)
{
  gchar *absolute_filename;

  if (g_path_is_absolute (filename) == FALSE)
  {
    gchar *current_dir = g_get_current_dir ();

    absolute_filename = g_build_filename (current_dir,
                                  filename, NULL);
    g_free (current_dir);
  }
  else
  {
    absolute_filename = g_strdup (filename);
  }


  if (g_file_test (absolute_filename,
                   G_FILE_TEST_IS_REGULAR))
  {
    if (_schedule)
    {
      People::CheckinSupervisor *checkin;

      _schedule->CreateDefault ();
      checkin = _schedule->GetCheckinSupervisor ();

      if (   g_str_has_suffix (filename, ".fff")
          || g_str_has_suffix (filename, ".FFF"))
      {
        checkin->ImportFFF (absolute_filename);
      }
      else
      {
        checkin->ImportCSV (absolute_filename);
      }
    }
  }
}

// --------------------------------------------------------------------------------
void Contest::LoadXmlDoc (xmlDoc *doc)
{
  _state = LOADING;

  {
    gboolean     score_stuffing_policy  = FALSE;
    gboolean     need_post_processing;
    const gchar *contest_keyword;

    xmlXPathInit ();

    {
      xmlXPathContext *xml_context = xmlXPathNewContext (doc);
      xmlXPathObject  *xml_object;
      xmlNodeSet      *xml_nodeset;

      {
        contest_keyword = "/CompetitionIndividuelle";
        xml_object = xmlXPathEval (BAD_CAST contest_keyword, xml_context);

        need_post_processing = FALSE;
        _team_event          = FALSE;
      }
      if (xml_object->nodesetval->nodeNr == 0)
      {
        xmlXPathFreeObject (xml_object);
        contest_keyword = "/BaseCompetitionIndividuelle";
        xml_object = xmlXPathEval (BAD_CAST contest_keyword, xml_context);

        need_post_processing = TRUE;
        _team_event          = FALSE;
      }
      if (xml_object->nodesetval->nodeNr == 0)
      {
        xmlXPathFreeObject (xml_object);
        contest_keyword = "/CompetitionParEquipes";
        xml_object = xmlXPathEval (BAD_CAST contest_keyword, xml_context);

        need_post_processing = FALSE;
        _team_event          = TRUE;
      }
      if (xml_object->nodesetval->nodeNr == 0)
      {
        xmlXPathFreeObject (xml_object);
        contest_keyword = "/BaseCompetitionParEquipes";
        xml_object = xmlXPathEval (BAD_CAST contest_keyword, xml_context);

        need_post_processing = TRUE;
        _team_event          = TRUE;
      }

      xml_nodeset = xml_object->nodesetval;

      if (xml_nodeset && xml_nodeset->nodeNr)
      {
        gchar *attr;

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "Couleur");
        if (attr)
        {
          _gdk_color = gdk_color_copy ((GdkColor *) (_color_list->data));
          if (gdk_color_parse (attr,
                               _gdk_color) == FALSE)
          {
            gdk_color_free (_gdk_color);
            _gdk_color = NULL;
          }
          xmlFree (attr);
        }
        if (_gdk_color == NULL)
        {
          ChooseColor ();
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "Championnat");
        if (attr)
        {
          g_free (_level);
          _level = g_strdup (attr);
          xmlFree (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "ID");
        if (attr)
        {
          g_free (_id);
          _id = g_strdup (attr);
          xmlFree (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "TitreLong");
        if (attr)
        {
          g_free (_name);
          _name = g_strdup (attr);
          xmlFree (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "Date");
        if (attr == NULL)
        {
          attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "DateDebut");
        }
        if (attr)
        {
          if (strlen (attr) > 0)
          {
            gchar **date = g_strsplit_set (attr,
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
          xmlFree (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "Appel");
        if (attr)
        {
          _checkin_time->Load (attr);
          xmlFree (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "Scratch");
        if (attr)
        {
          _scratch_time->Load (attr);
          xmlFree (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "Debut");
        if (attr)
        {
          _start_time->Load (attr);
          xmlFree (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "Arme");
        if (attr)
        {
          _weapon = Weapon::GetFromXml (attr);
          xmlFree (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "Sexe");
        if (attr)
        {
          guint i;

          for (i = 0; i < _nb_gender; i++)
          {
            if (strcmp (attr, gender_xml_image[i]) == 0)
            {
              _gender = i;
              break;
            }
          }
          if (i == _nb_gender)
          {
            for (i = 0; i < _nb_gender; i++)
            {
              if (strcmp (attr, gender_xml_alias[i]) == 0)
              {
                _gender = i;
                break;
              }
            }
          }

          xmlFree (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "Categorie");
        if (attr)
        {
          guint i;

          for (i = 0; i < _nb_category; i++)
          {
            if (strcmp (attr, category_xml_alias[i]) == 0)
            {
              _category = i;
              break;
            }
          }
          if (i == _nb_category)
          {
            for (i = 0; i < _nb_category; i++)
            {
              if (strcmp (attr, category_xml_image[i]) == 0)
              {
                _category = i;
                break;
              }
            }
          }
          xmlFree (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "Organisateur");
        if (attr)
        {
          g_free (_organizer);
          _organizer = g_strdup (attr);
          xmlFree (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "URLorganisateur");
        if (attr)
        {
          g_free (_web_site);
          _web_site = g_strdup (attr);
          xmlFree (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "Lieu");
        if (attr)
        {
          g_free (_location);
          _location = g_strdup (attr);
          xmlFree (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "ScoreAleatoire");
        if (attr)
        {
          score_stuffing_policy = (gboolean) atoi (attr);
          xmlFree (attr);
        }

        _minimum_team_size->Load      (xml_nodeset->nodeTab[0]);
        _default_classification->Load (xml_nodeset->nodeTab[0]);
        _manual_classification->Load  (xml_nodeset->nodeTab[0]);
      }
      xmlXPathFreeObject  (xml_object);

      GetUnknownAttributes (contest_keyword,
                            xml_context);
      xmlXPathFreeContext (xml_context);
    }

    Spread ();

    _schedule->Load (doc,
                     contest_keyword,
                     _referees_list);

    if (score_stuffing_policy)
    {
      _schedule->SetScoreStuffingPolicy (score_stuffing_policy);
    }

    _schedule->SetTeamEvent (_team_event);

    if (need_post_processing)
    {
      People::CheckinSupervisor *checkin = _schedule->GetCheckinSupervisor ();

      checkin->ConvertFromBaseToResult ();
    }
  }

  FillInProperties ();

  _state = OPERATIONAL;
  _schedule->OnLoadingCompleted ();

  _referees_list->Disclose ("Referee");
  _referees_list->Spread ();
}

// --------------------------------------------------------------------------------
Contest::~Contest ()
{
  _state = LEAVING;

  // www
  if (_filename)
  {
    gchar *base_name = g_path_get_basename (_filename);
    gchar *www_file  = g_build_filename (Global::_share_dir, "webserver", "LightTPD", "www", "cotcot", base_name, NULL);

    g_unlink (www_file);

    g_free (base_name);
    g_free (www_file);
  }

  g_free (_level);
  g_free (_id);
  g_free (_name);
  g_free (_filename);
  g_free (_organizer);
  g_free (_web_site);
  g_free (_location);

  Object::TryToRelease (_manual_classification);
  Object::TryToRelease (_minimum_team_size);
  Object::TryToRelease (_default_classification);

  Object::TryToRelease (_checkin_time);
  Object::TryToRelease (_scratch_time);
  Object::TryToRelease (_start_time);

  gdk_color_free (_gdk_color);

  Object::TryToRelease (_referees_list);

  Object::TryToRelease (_schedule);

  if (_save_timeout_id > 0)
  {
    g_source_remove (_save_timeout_id);
  }

  if (_tournament)
  {
    _tournament->OnContestDeleted (this);
  }
}

// --------------------------------------------------------------------------------
void Contest::FeedParcel (Net::Message *parcel)
{
  gchar *color = gdk_color_to_string (_gdk_color);

  parcel->Set ("id",       _id);
  parcel->Set ("color",    color);
  parcel->Set ("weapon",   _weapon->GetImage ());
  parcel->Set ("gender",   gender_image[_gender]);
  parcel->Set ("category", category_image[_category]);

  g_free (color);
}

// --------------------------------------------------------------------------------
gchar *Contest::GetFilename ()
{
  return _filename;
}

// --------------------------------------------------------------------------------
Module::State Contest::GetState ()
{
  return _state;
}

// --------------------------------------------------------------------------------
void Contest::AddFencer (Player *fencer,
                         guint   rank)
{
  if (_schedule)
  {
    People::CheckinSupervisor *checkin = _schedule->GetCheckinSupervisor ();

    if (checkin)
    {
      Player::AttributeId ranking_attr ("ranking");

      fencer->SetAttributeValue (&ranking_attr,
                                 rank);
      checkin->Add (fencer);
    }
  }
}

// --------------------------------------------------------------------------------
void Contest::AddFencer (Player *fencer)
{
  if (_schedule)
  {
    People::CheckinSupervisor *checkin = _schedule->GetCheckinSupervisor ();

    if (checkin)
    {
      checkin->Add (fencer);
    }
  }
}

// --------------------------------------------------------------------------------
Contest *Contest::Create ()
{
  Contest *contest = new Contest ();

  contest->_schedule->SetScoreStuffingPolicy (FALSE);

  contest->FillInProperties ();
  contest->_schedule->CreateDefault ();

  contest->_schedule->SetTeamEvent (FALSE);

  {
    gtk_dialog_run (GTK_DIALOG (contest->_properties_dialog));
    contest->ReadProperties ();
    gtk_widget_hide (contest->_properties_dialog);
  }

  if (contest->_read_only == FALSE)
  {
    GtkWidget *chooser = GTK_WIDGET (gtk_file_chooser_dialog_new (gettext ("Choose a file..."),
                                                                  NULL,
                                                                  GTK_FILE_CHOOSER_ACTION_SAVE,
                                                                  GTK_STOCK_CANCEL,  GTK_RESPONSE_CANCEL,
                                                                  GTK_STOCK_SAVE_AS, GTK_RESPONSE_ACCEPT,
                                                                  NULL));

    contest->GetSaveFileName (chooser,
                              "default_dir_name");
    gtk_widget_destroy (chooser);
  }

  contest->Save ();

  return contest;
}

// --------------------------------------------------------------------------------
Contest *Contest::Duplicate ()
{
  Contest *contest = new Contest (TRUE);

  contest->_schedule->CreateDefault (TRUE);
  contest->_derived = TRUE;

  if (_level)
  {
    contest->_level = g_strdup (_level);
  }
  if (_id)
  {
    contest->_id = g_strdup (_id);
  }
  contest->_name       = g_strdup (_name);
  contest->_organizer  = g_strdup (_organizer);
  contest->_web_site   = g_strdup (_web_site);
  contest->_location   = g_strdup (_location);
  contest->_category   = _category;
  contest->_weapon     = _weapon;
  contest->_gender     = _gender;
  contest->_day        = _day;
  contest->_month      = _month;
  contest->_year       = _year;
  contest->_tournament = _tournament;
  contest->_team_event = _team_event;
  contest->_schedule->SetTeamEvent (_team_event);

  _checkin_time->Copy (contest->_checkin_time);
  _scratch_time->Copy (contest->_scratch_time);
  _start_time->Copy   (contest->_start_time);

  contest->_minimum_team_size->Copy (_minimum_team_size);

  contest->FillInProperties ();

  return contest;
}

// --------------------------------------------------------------------------------
void Contest::LatchPlayerList ()
{
  People::CheckinSupervisor *checkin = _schedule->GetCheckinSupervisor ();

  if (checkin)
  {
    checkin->OnListChanged ();
  }
}

// --------------------------------------------------------------------------------
Player *Contest::GetRefereeFromRef (guint ref)
{
  if (ref)
  {
    GSList *current = _referees_list->GetList ();

    while (current)
    {
      Player *player = (Player *) current->data;

      if (player->GetRef () == ref)
      {
        return player;
      }
      current = g_slist_next (current);
    }
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void Contest::ChooseColor ()
{
  gint color_to_use;

  color_to_use = g_key_file_get_integer (Global::_user_config->_key_file,
                                         "Competiton",
                                         "color_to_use",
                                         NULL);
  if (color_to_use >= (gint) (g_list_length (_color_list)))
  {
    color_to_use = 0;
  }

  color_to_use++;
  if (color_to_use >= (gint) (g_list_length (_color_list)))
  {
    color_to_use = 0;
  }
  g_key_file_set_integer (Global::_user_config->_key_file,
                          "Competiton",
                          "color_to_use",
                          color_to_use);

  _gdk_color = gdk_color_copy ((GdkColor *) g_list_nth_data (_color_list,
                                                             color_to_use));
}

// --------------------------------------------------------------------------------
gboolean Contest::OnSaveTimeout (Contest *contest)
{
  if (contest->_filename)
  {
    contest->Save ();
  }

  contest->_save_timeout_id = 0;
  return FALSE;
}

// --------------------------------------------------------------------------------
void Contest::MakeDirty ()
{
  gtk_widget_set_sensitive (_glade->GetWidget ("save_toolbutton"),
                            TRUE);

  if (_save_timeout_id > 0)
  {
    g_source_remove (_save_timeout_id);
  }

  _save_timeout_id = g_timeout_add_seconds (5,
                                            (GSourceFunc) OnSaveTimeout,
                                            this);
}

// --------------------------------------------------------------------------------
void Contest::Init ()
{
  GdkColor *color;

  color = g_new (GdkColor, 1);
  gdk_color_parse ("#EED680", color); // accent yellow
  _color_list = g_list_append (_color_list, color);

  color = g_new (GdkColor, 1);
  gdk_color_parse ("#E0B6AF", color); // red hilight
  _color_list = g_list_append (_color_list, color);

  color = g_new (GdkColor, 1);
  gdk_color_parse ("#ADA7C8", color); // purple hilight
  _color_list = g_list_append (_color_list, color);

  color = g_new (GdkColor, 1);
  gdk_color_parse ("#9DB8D2", color); // blue hilight
  _color_list = g_list_append (_color_list, color);

  color = g_new (GdkColor, 1);
  gdk_color_parse ("#83A67F", color); // green medium
  _color_list = g_list_append (_color_list, color);

  color = g_new (GdkColor, 1);
  gdk_color_parse ("#DF421E", color); // accent red
  _color_list = g_list_append (_color_list, color);

  color = g_new (GdkColor, 1);
  gdk_color_parse ("#826647", color); // Basic 3D hilight
  _color_list = g_list_append (_color_list, color);
}

// --------------------------------------------------------------------------------
void Contest::Cleanup ()
{
  g_list_foreach (_color_list,
                  (GFunc) g_free,
                  NULL);
  g_list_free (_color_list);

  _color_list = NULL;
}

// --------------------------------------------------------------------------------
void Contest::OnPlugged ()
{
  FlashCode *flash_code;
  gchar     *url;

  _tournament = dynamic_cast <Tournament *> (_owner);

  flash_code = _tournament->GetFlashCode ();
  url = flash_code->GetText ();
  SetFlashRef (flash_code->GetText ());
  g_free (url);
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

  gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("location_entry")),
                      _location);

  gtk_combo_box_set_active (GTK_COMBO_BOX (_weapon_combo),
                            _weapon->GetIndex ());

  gtk_combo_box_set_active (GTK_COMBO_BOX (_gender_combo),
                            _gender);

  gtk_combo_box_set_active (GTK_COMBO_BOX (_category_combo),
                            _category);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("allow_radiobutton")),
                                _schedule->ScoreStuffingIsAllowed ());

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("individual_radiobutton")),
                                (_team_event == FALSE));

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

  entry = GTK_ENTRY (_glade->GetWidget ("location_entry"));
  str = (gchar *) gtk_entry_get_text (entry);
  g_free (_location);
  _location = g_strdup (str);

  _weapon   = Weapon::GetFromIndex (gtk_combo_box_get_active (GTK_COMBO_BOX (_weapon_combo)));
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

  ReadTeamProperty ();

  {
    if (_name && *_name != 0)
    {
      g_key_file_set_string (Global::_user_config->_key_file,
                             "Competiton",
                             "default_name",
                             _name);
    }

    if (_organizer && *_organizer != 0)
    {
      g_key_file_set_string (Global::_user_config->_key_file,
                             "Competiton",
                             "default_organizer",
                             _organizer);
    }

    if (_web_site && *_web_site != 0)
    {
      g_key_file_set_string (Global::_user_config->_key_file,
                             "Competiton",
                             "default_web_site",
                             _web_site);
    }

    if (_location && *_location != 0)
    {
      g_key_file_set_string (Global::_user_config->_key_file,
                             "Competiton",
                             "default_location",
                             _location);
    }
  }

  _schedule->ApplyNewConfig ();
  Spread ();
  DisplayProperties ();
}

// --------------------------------------------------------------------------------
void Contest::ReadTeamProperty ()
{
  gboolean event = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("team_radiobutton")));

  if (event != _team_event)
  {
    _team_event = event;
    _schedule->SetTeamEvent (_team_event);
  }
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
                        _weapon->GetImage ());
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
void Contest::AttachTo (GtkNotebook *to)
{
  gtk_notebook_append_page (to,
                            GetRootWidget (),
                            _glade->GetWidget ("notebook_title"));

  if (_derived == FALSE)
  {
    gtk_notebook_set_current_page (to,
                                   -1);
  }

  gtk_notebook_set_tab_reorderable (to,
                                    GetRootWidget (),
                                    TRUE);

  DisplayProperties ();
}

// --------------------------------------------------------------------------------
void Contest::Publish ()
{
  if (_tournament && (_schedule->ScoreStuffingIsAllowed () == FALSE))
  {
    {
      EcoSystem *ecosystem = _tournament->GetEcosystem ();

      ecosystem->UploadFile (_filename);
    }
  }
  //if (_checkin_time->IsEqualTo (_scratch_time))
  //{
  //return;
  //}
  //if (_scratch_time->IsEqualTo (_start_time))
  //{
  //return;
  //}
}

// --------------------------------------------------------------------------------
void Contest::Save ()
{
  if (_filename)
  {
    // Regular
    Save (_filename);

    // www
    {
      gchar *base_name = g_path_get_basename (_filename);
      gchar *www_file  = g_build_filename (Global::_share_dir, "webserver", "LightTPD", "www", "cotcot", base_name, NULL);

      Save (www_file);
      g_free (base_name);
      g_free (www_file);
    }

    // Backup
    if (_tournament)
    {
      const gchar *location = _tournament->GetBackupLocation ();

      if (location)
      {
        gchar *base_name   = g_path_get_basename (_filename);
        gchar *backup_file = g_build_filename (location, base_name, NULL);

        Save (backup_file);
        g_free (base_name);
        g_free (backup_file);
      }
    }

    Publish ();
  }
}

// --------------------------------------------------------------------------------
void Contest::SaveHeader (xmlTextWriter *xml_writer)
{
  if (xml_writer)
  {
    xmlTextWriterSetIndent     (xml_writer,
                                TRUE);
    xmlTextWriterStartDocument (xml_writer,
                                NULL,
                                "UTF-8",
                                NULL);

    if (_team_event)
    {
      xmlTextWriterStartDTD (xml_writer,
                             BAD_CAST "CompetitionParEquipes",
                             NULL,
                             NULL);
    }
    else
    {
      xmlTextWriterStartDTD (xml_writer,
                             BAD_CAST "CompetitionIndividuelle",
                             NULL,
                             NULL);
    }
    xmlTextWriterEndDTD (xml_writer);

    xmlTextWriterStartComment (xml_writer);
    xmlTextWriterWriteFormatString (xml_writer, "\n");
    xmlTextWriterWriteFormatString (xml_writer, "           By BellePoule (V%s.%s%s)\n", VERSION, VERSION_REVISION, VERSION_MATURITY);
    xmlTextWriterWriteFormatString (xml_writer, "\n");
    xmlTextWriterWriteFormatString (xml_writer, "   http://betton.escrime.free.fr/index.php/bellepoule\n");
    xmlTextWriterEndComment (xml_writer);

    if (_team_event)
    {
      xmlTextWriterStartElement (xml_writer,
                                 BAD_CAST "CompetitionParEquipes");
    }
    else
    {
      xmlTextWriterStartElement (xml_writer,
                                 BAD_CAST "CompetitionIndividuelle");
    }

    {
      gchar *color = gdk_color_to_string  (_gdk_color);

      if (color)
      {
        // #rrrrggggbbbb ==> #rrggbb
        color[3] = color[5];
        color[4] = color[6];
        color[5] = color[9];
        color[6] = color[10];
        color[7] = 0;
        xmlTextWriterWriteAttribute (xml_writer,
                                     BAD_CAST "Couleur",
                                     BAD_CAST color);
        g_free (color);
      }
    }

    if (_level)
    {
      xmlTextWriterWriteAttribute (xml_writer,
                                   BAD_CAST "Championnat",
                                   BAD_CAST _level);
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
                                 BAD_CAST _weapon->GetXmlImage ());
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
                                       BAD_CAST "Date",
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
                           "Debut");
    }
    xmlTextWriterWriteAttribute (xml_writer,
                                 BAD_CAST "TitreLong",
                                 BAD_CAST _name);
    xmlTextWriterWriteAttribute (xml_writer,
                                 BAD_CAST "URLorganisateur",
                                 BAD_CAST _web_site);
    xmlTextWriterWriteFormatAttribute (xml_writer,
                                       BAD_CAST "ScoreAleatoire",
                                       "%d", _schedule->ScoreStuffingIsAllowed ());
    // Team configuration
    {
      _minimum_team_size->Save      (xml_writer);
      _default_classification->Save (xml_writer);
      _manual_classification->Save  (xml_writer);
    }

    xmlTextWriterWriteAttribute (xml_writer,
                                 BAD_CAST "Lieu",
                                 BAD_CAST _location);
  }

  _schedule->SavePeoples (xml_writer,
                          _referees_list);
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
      SaveHeader (xml_writer);

      _schedule->Save (xml_writer);

      xmlTextWriterEndElement (xml_writer);
      xmlTextWriterEndDocument (xml_writer);

      xmlFreeTextWriter (xml_writer);
    }

    gtk_widget_set_sensitive (_glade->GetWidget ("save_toolbutton"),
                              FALSE);
  }
}

// --------------------------------------------------------------------------------
void Contest::DumpToHTML (gchar *filename)
{
  if (filename)
  {
    FILE *file = g_fopen (filename, "w");

    {
      fprintf (file,
               "<html>\n"
               "  <head>\n"
               "    <Title>%s %s - %s - %s - %s</Title>\n"
               "    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>\n"
#include "css.h"
               "  </head>\n\n",
               GetName (),
               GetDate (),
               _weapon->GetImage (),
               GetGender (),
               GetCategory ());

      fprintf (file,
               "  <body onLoad=\"javascript:OnLoad ();\">\n"
               "    <div id=\'main_div\'>\n"
               "      <div class=\"Title\">\n"
               "        <center>\n"
               "          <h1>%s</h1>\n"
               "          <h1>%s</h1>\n"
               "          <h1>%s - %s - %s</h1>\n"
               "        </center>\n"
               "      <div>\n"
               "\n",
               GetName (),
               GetDate (),
               _weapon->GetImage (),
               GetGender (),
               GetCategory ());
    }

    _schedule->DumpToHTML (file);

    {
      fprintf (file,
               "      <div class=\"Footer\">\n"
               "        <h6>Managed by BellePoule <a href=\"http://betton.escrime.free.fr/index.php/bellepoule\">http://betton.escrime.free.fr/index.php/bellepoule</a><h6/>\n"
               "      <div>\n"
               "    </div>\n"
               "  </body>\n");

      fprintf (file, "</html>\n");
      fclose (file);
    }
  }
}

// --------------------------------------------------------------------------------
gchar *Contest::GetSaveFileName (GtkWidget   *chooser,
                                 const gchar *config_key,
                                 gint        *choice)
{
  gint response;

  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (chooser),
                                                  TRUE);

  {
    GtkFileFilter *filter = gtk_file_filter_new ();

    gtk_file_filter_set_name (filter,
                              gettext ("BellePoule files (.cotcot)"));
    gtk_file_filter_add_pattern (filter,
                                 "*.cotcot");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser),
                                 filter);
  }

  {
    GtkFileFilter *filter = gtk_file_filter_new ();

    gtk_file_filter_set_name (filter,
                              gettext ("All files"));
    gtk_file_filter_add_pattern (filter,
                                 "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser),
                                 filter);
  }

  {
    gchar *last_dirname = g_key_file_get_string (Global::_user_config->_key_file,
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

  {
    char *name = GetDefaultFileName ();

    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (chooser),
                                       name);
    g_free (name);
  }

  response = gtk_dialog_run (GTK_DIALOG (chooser));

  if (response == GTK_RESPONSE_ACCEPT)
  {
    _filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));

    if (_filename)
    {
      gtk_widget_set_tooltip_text (_glade->GetWidget ("save_toolbutton"),
                                   _filename);

      {
        gchar *dirname = g_path_get_dirname (_filename);

        g_key_file_set_string (Global::_user_config->_key_file,
                               "Competiton",
                               config_key,
                               dirname);
        g_free (dirname);
      }

      {
        gchar *uri = g_filename_to_uri (_filename,
                                        NULL,
                                        NULL);

        gtk_recent_manager_add_item (gtk_recent_manager_get_default (),
                                     uri);
        g_free (uri);
      }
    }
  }

  if (choice)
  {
    *choice = response;
  }

  return _filename;
}

// --------------------------------------------------------------------------------
gboolean Contest::OnHttpPost (const gchar *command,
                              const gchar **resource,
                              const gchar *data)
{
  return _schedule->OnHttpPost (command,
                                resource,
                                data);
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
  _schedule->DrawBookPage (operation,
                           context,
                           page_nr);
}

// --------------------------------------------------------------------------------
void Contest::DrawPage (GtkPrintOperation *operation,
                        GtkPrintContext   *context,
                        gint               page_nr)
{
  GooCanvas *canvas = Canvas::CreatePrinterCanvas (context);
  cairo_matrix_t *operation_matrix = (cairo_matrix_t *) g_object_get_data (G_OBJECT (operation),
                                                                           "operation_matrix");

  if (operation_matrix)
  {
    cairo_matrix_t own_matrix;
    cairo_matrix_t result_matrix;

    goo_canvas_item_get_transform (goo_canvas_get_root_item (canvas),
                                   &own_matrix);
    cairo_matrix_multiply (&result_matrix,
                           operation_matrix,
                           &own_matrix);
    goo_canvas_item_set_transform (goo_canvas_get_root_item (canvas),
                                   &result_matrix);
  }

  goo_canvas_rect_new (goo_canvas_get_root_item (canvas),
                       0.0, 0.0,
                       100.0, PRINT_HEADER_HEIGHT,
                       "stroke-color", "grey",
                       "fill-color", gdk_color_to_string (_gdk_color),
                       "line-width", 0.3,
                       NULL);

  {
    goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                         _name,
                         10.0, 0.8,
                         20.0,
                         GTK_ANCHOR_NORTH,
                         "alignment", PANGO_ALIGN_CENTER,
                         "fill-color", "black",
                         "font", BP_FONT "Bold 2.5px", NULL);
  }

  {
    char *text = g_strdup_printf ("%s - %s - %s", _weapon->GetImage (),
                                  gettext (gender_image[_gender]),
                                  gettext (category_image[_category]));
    goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                         text,
                         50.5, 3.5,
                         -1.0,
                         GTK_ANCHOR_CENTER,
                         "fill-color", "black",
                         "font", BP_FONT "Bold 4px", NULL);
    goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                         text,
                         50.0, 3.0,
                         -1.0,
                         GTK_ANCHOR_CENTER,
                         "fill-color", "white",
                         "font", BP_FONT "Bold 4px", NULL);
    g_free (text);
  }

  {
    goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                         (const char *) g_object_get_data (G_OBJECT (operation), "Print::PageName"),
                         50.0, 7.5,
                         -1.0,
                         GTK_ANCHOR_CENTER,
                         "fill-color", "black",
                         "font", BP_FONT "Bold 4px", NULL);
  }

  if (_organizer)
  {
    goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                         _organizer,
                         98.0, 2.0,
                         -1.0,
                         GTK_ANCHOR_EAST,
                         "fill-color", "black",
                         "font", BP_FONT "Bold 2.5px", NULL);
  }

  {
    goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                         GetDate (),
                         98.0, 5.0,
                         -1.0,
                         GTK_ANCHOR_EAST,
                         "fill-color", "black",
                         "font", BP_FONT "Bold 2.5px", NULL);
  }

  goo_canvas_render (canvas,
                     gtk_print_context_get_cairo_context (context),
                     NULL,
                     1.0);

  gtk_widget_destroy (GTK_WIDGET (canvas));

  if (operation_matrix == NULL)
  {
    gdouble  paper_w = gtk_print_context_get_width  (context);
    cairo_t *cr      = gtk_print_context_get_cairo_context (context);

    cairo_translate (cr,
                     0.0,
                     (PRINT_HEADER_HEIGHT+2) * paper_w  / 100);
  }
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
  if ((_filename == NULL) && (_read_only == FALSE))
  {
    GtkWidget *chooser = GTK_WIDGET (gtk_file_chooser_dialog_new (gettext ("Choose a file..."),
                                                                  NULL,
                                                                  GTK_FILE_CHOOSER_ACTION_SAVE,
                                                                  GTK_STOCK_CANCEL,  GTK_RESPONSE_CANCEL,
                                                                  GTK_STOCK_SAVE_AS, GTK_RESPONSE_ACCEPT,
                                                                  NULL));
    GetSaveFileName (chooser,
                     "default_dir_name");
    gtk_widget_destroy (chooser);
  }

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
  gtk_dialog_run (GTK_DIALOG (_properties_dialog));
  ReadProperties ();
  MakeDirty ();
  gtk_widget_hide (_properties_dialog);
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
  GtkWidget *dialog = _glade->GetWidget ("calendar_dialog");

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == TRUE)
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
  gtk_widget_hide (dialog);
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
  gint choice;

  if ((_filename == NULL) && (_read_only == FALSE))
  {
    GtkWidget *chooser = GTK_WIDGET (gtk_file_chooser_dialog_new (gettext ("Choose a file..."),
                                                                  NULL,
                                                                  GTK_FILE_CHOOSER_ACTION_SAVE,
                                                                  GTK_STOCK_CANCEL,                 GTK_RESPONSE_CANCEL,
                                                                  gettext ("Close without saving"), GTK_RESPONSE_REJECT,
                                                                  GTK_STOCK_SAVE_AS,                GTK_RESPONSE_ACCEPT,
                                                                  NULL));
    GetSaveFileName (chooser,
                     "default_dir_name",
                     &choice);
    gtk_widget_destroy (chooser);
  }

  Save ();

  if (choice != GTK_RESPONSE_CANCEL)
  {
    Release ();
  }
}

// --------------------------------------------------------------------------------
gchar *Contest::GetOrganizer ()
{
  return _organizer;
}

// --------------------------------------------------------------------------------
Weapon *Contest::GetWeapon ()
{
  return _weapon;
}

// --------------------------------------------------------------------------------
gboolean Contest::IsTeamEvent ()
{
  return _team_event;
}

// --------------------------------------------------------------------------------
gchar *Contest::GetName ()
{
  return _name;
}

// --------------------------------------------------------------------------------
gchar *Contest::GetId ()
{
  return _id;
}

// --------------------------------------------------------------------------------
gchar *Contest::GetDefaultFileName ()
{
  return g_strdup_printf ("%s-%s-%s-%s.cotcot", _name,
                          _weapon->GetImage (),
                          gettext (gender_image[_gender]),
                          gettext (category_image[_category]));
}

// --------------------------------------------------------------------------------
gchar *Contest::GetGenderCode ()
{
  return (char *) gender_xml_image[_gender];
}

// --------------------------------------------------------------------------------
gchar *Contest::GetGender ()
{
  return (char *) gettext (gender_image[_gender]);
}

// --------------------------------------------------------------------------------
gchar *Contest::GetCategory ()
{
  return gettext (category_image[_category]);
}

// --------------------------------------------------------------------------------
gchar *Contest::GetDate ()
{
  static gchar image[] = "Wide enough memory buffer to store the printable date";
  GDate *date = g_date_new_dmy (_day,
                                (GDateMonth) _month,
                                _year);

  g_date_strftime (image,
                   strlen (image)+1,
                   "%x",
                   date);
  g_date_free (date);

  return image;
}

// --------------------------------------------------------------------------------
void Contest::OnBeginPrint (GtkPrintOperation *operation,
                            GtkPrintContext   *context)
{
  _schedule->PrepareBookPrint (operation,
                               context);
}

// --------------------------------------------------------------------------------
void Contest::OnEndPrint (GtkPrintOperation *operation,
                          GtkPrintContext   *context)
{
  _schedule->StopBookPrint (operation,
                            context);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_team_radiobutton_toggled (GtkToggleToolButton *widget,
                                                             Object              *owner)
{
  Contest *c = dynamic_cast <Contest *> (owner);

  c->ReadTeamProperty ();
}
