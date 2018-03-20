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
#include "util/user_config.hpp"
#include "util/attribute.hpp"
#include "util/attribute_desc.hpp"
#include "util/filter.hpp"
#include "util/glade.hpp"
#include "util/data.hpp"
#include "util/flash_code.hpp"
#include "actors/player_factory.hpp"
#include "network/greg_uploader.hpp"
#include "network/message.hpp"
#include "actors/checkin.hpp"
#include "actors/referees_list.hpp"
#include "actors/player_factory.hpp"
#include "rounds/checkin/checkin_supervisor.hpp"
#include "publication.hpp"
#include "schedule.hpp"


#include "application/version.h"
#include "application/weapon.hpp"
#include "category.hpp"

#include "ask_fred.hpp"
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

const gchar *Contest::level_image[_nb_level] =
{
  N_ ("League"),
  N_ ("Regional"),
  N_ ("Open"),
  N_ ("World cup"),
  N_ ("National championships"),
  N_ ("World championships"),
  N_ ("Other")
};

const gchar *Contest::level_xml_image[_nb_level] =
{
  "L",
  "Z",
  "CN",
  "A",
  "F",
  "IE",
  "X"
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
Contest::Contest (GList    *advertisers,
                  gboolean  for_duplication)
  : Object ("Contest"),
    Module ("contest.glade")
{
  Disclose ("Competition");

  _save_timeout_id = 0;

  _fie_id = g_strdup_printf ("%x", GetNetID ());

   _read_only   = FALSE;
   _authority   = NULL;
   _filename    = NULL;
   _tournament  = NULL;
   _weapon      = Weapon::GetDefault ();
   _category    = new Category (_glade->GetWidget ("category_combobox"));
   _level       = 0;
   _gender      = 0;
   _team_event  = FALSE;
   _derived     = FALSE;
   _source      = NULL;
   _advertisers = advertisers;

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

  _referees_list = new People::RefereesList ();

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
                              _advertisers,
                              _minimum_team_size,
                              _manual_classification,
                              _default_classification);

    Plug (_schedule,
          _glade->GetWidget ("schedule_viewport"),
          GTK_TOOLBAR (_glade->GetWidget ("contest_toolbar")),
          GTK_CONTAINER (_glade->GetWidget ("formula_alignment")));
  }

  {
    GList *current = Weapon::GetList ();

    _weapon_combo = _glade->GetWidget ("weapon_combo");
    while (current)
    {
      Weapon *weapon = (Weapon *) current->data;

      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (_weapon_combo),
                                      weapon->GetImage ());
      current = g_list_next (current);
    }
  }

  {
    _gender_combo = _glade->GetWidget ("gender_combo");
    for (guint i = 0; i < _nb_gender; i ++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (_gender_combo), gettext (gender_image[i]));
    }
  }

  {
    _level_combo = _glade->GetWidget ("level_combo");
    for (guint i = 0; i < _nb_level; i ++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (_level_combo), gettext (level_image[i]));
    }
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
                                  (gchar *)       current_attr->name);
        }

        current_attr = current_attr->next;
      }
    }

    xmlXPathFreeObject  (xml_object);
  }
}

// --------------------------------------------------------------------------------
void Contest::LoadAskFred (AskFred::Event *askfred,
                           const gchar    *dirname)
{
  g_free (_name);
  _name = g_strdup (askfred->_name);

  g_free (_location);
  _location = g_strdup (askfred->_location);

  g_free (_location);
  _location = g_strdup (askfred->_location);

  g_free (_organizer);
  _organizer = g_strdup (askfred->_organizer);

  g_free (_source);
  _source = g_strdup ("AskFred");

  for (guint i = 0; i < _nb_gender; i++)
  {
    if (g_strcmp0 (askfred->_gender, gender_xml_image[i]) == 0)
    {
      _gender = i;
      break;
    }
  }

  _weapon = Weapon::GetFromXml (askfred->_weapon);

  _category->ParseXml (askfred->_category);

  _year   = askfred->_year;
  _month  = askfred->_month;
  _day    = askfred->_day;

  _checkin_time->_hour   = askfred->_hour;
  _checkin_time->_minute = askfred->_minute;

  {
    GDateTime *date;
    GDateTime *scratch;
    GDateTime *start;

    date = g_date_time_new_local (_year,
                                  _month,
                                  _day,
                                  _checkin_time->_hour,
                                  _checkin_time->_minute,
                                  0);

    scratch = g_date_time_add_minutes (date,    15);
    start   = g_date_time_add_minutes (scratch, 15);

    _scratch_time->_hour   = g_date_time_get_hour   (scratch);
    _scratch_time->_minute = g_date_time_get_minute (scratch);

    _start_time->_hour   = g_date_time_get_hour   (start);
    _start_time->_minute = g_date_time_get_minute (start);

    g_date_time_unref (start);
    g_date_time_unref (scratch);
    g_date_time_unref (date);
  }

  if (_schedule)
  {
    AttributeDesc             *league_desc = AttributeDesc::GetDescFromCodeName ("league");
    People::CheckinSupervisor *checkin;
    GList                     *current  = askfred->_fencers;

    league_desc->_favorite_look = AttributeDesc::SHORT_TEXT;
    _schedule->CreateDefault ();
    _schedule->SetTeamEvent (FALSE);
    league_desc->_favorite_look = AttributeDesc::GRAPHICAL;

    checkin = _schedule->GetCheckinSupervisor ();

    {
      Filter *filter = checkin->GetFilter ();

      filter->ShowAttribute ("rating");
      filter->UpdateAttrList (FALSE);
      filter->Release ();
    }

    while (current)
    {
      AskFred::Fencer     *fencer = (AskFred::Fencer *) current->data;
      Player              *player = PlayerFactory::CreatePlayer ("Fencer");
      Player::AttributeId  attr_id ("");

      attr_id._name = (gchar *) "name";
      player->SetAttributeValue (&attr_id, fencer->_last_name);

      attr_id._name = (gchar *) "first_name";
      player->SetAttributeValue (&attr_id, fencer->_first_name);

      attr_id._name = (gchar *) "birth_date";
      player->SetAttributeValue (&attr_id, fencer->_birth);

      attr_id._name = (gchar *) "gender";
      player->SetAttributeValue (&attr_id, fencer->_gender);

      attr_id._name = (gchar *) "club";
      player->SetAttributeValue (&attr_id, fencer->_club->_name);

      attr_id._name = (gchar *) "league";
      player->SetAttributeValue (&attr_id, fencer->_club->_division);

      attr_id._name = (gchar *) "licence";
      player->SetAttributeValue (&attr_id, fencer->_licence);

      attr_id._name = (gchar *) "ranking";
      player->SetAttributeValue (&attr_id, fencer->GetRanking (_weapon->GetXmlImage ()));

      attr_id._name = (gchar *) "rating";
      player->SetAttributeValue (&attr_id, fencer->GetRating (_weapon->GetXmlImage ()));

      checkin->Add (player);
      player->Release ();

      current = g_list_next (current);
    }
  }

  FillInProperties ();
}

// --------------------------------------------------------------------------------
void Contest::LoadXml (const gchar *filename)
{
  if (g_path_is_absolute (filename) == FALSE)
  {
    gchar *current_dir = g_get_current_dir ();

    SetFilename (g_build_filename (current_dir,
                                   filename, NULL));
    g_free (current_dir);
  }
  else
  {
    SetFilename (g_strdup (filename));
  }


  if (g_file_test (_filename,
                   G_FILE_TEST_IS_REGULAR))
  {
    xmlDoc *doc = xmlParseFile (filename);

    if (doc)
    {
      LoadXmlDoc (doc);

      AddFileToRecentManager (filename);

      if (g_str_has_suffix (_filename,
                            ".cotcot") == FALSE)
      {
        SetFilename (NULL);
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
          g_free (_authority);
          _authority = g_strdup (attr);
          xmlFree (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "ID");
        if (attr)
        {
          g_free (_fie_id);
          _fie_id = g_strdup (attr);
          xmlFree (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "NetID");
        if (attr)
        {
          _parcel->SetNetID (g_ascii_strtoull (attr,
                                               NULL,
                                               16));
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
            if (g_strcmp0 (attr, gender_xml_image[i]) == 0)
            {
              _gender = i;
              break;
            }
          }
          if (i == _nb_gender)
          {
            for (i = 0; i < _nb_gender; i++)
            {
              if (g_strcmp0 (attr, gender_xml_alias[i]) == 0)
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
          _category->ParseXml (attr);
          xmlFree (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "Niveau");
        if (attr)
        {
          for (guint i = 0; i < _nb_level; i++)
          {
            if (g_strcmp0 (attr, level_xml_image[i]) == 0)
            {
              _level = i;
              break;
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

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "Source");
        if (attr)
        {
          g_free (_source);
          _source = g_strdup (attr);
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

    _referees_list->SetWeapon (_weapon);

    {
      AttributeDesc *league_desc = AttributeDesc::GetDescFromCodeName ("league");

      if (_source && (g_strcmp0 (_source, "AskFred") == 0))
      {
        league_desc->_favorite_look = AttributeDesc::SHORT_TEXT;
      }

      _schedule->Load (doc,
                       contest_keyword,
                       _referees_list);

      league_desc->_favorite_look = AttributeDesc::GRAPHICAL;
    }

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

    if (_source && (g_strcmp0 (_source, "AskFred") == 0))
    {
      People::CheckinSupervisor *checkin = _schedule->GetCheckinSupervisor ();
      Filter                    *filter  = checkin->GetFilter ();

      filter->ShowAttribute ("rating");
      filter->UpdateAttrList (FALSE);
      filter->Release ();
    }
  }

  FillInProperties ();

  _state = OPERATIONAL;
  _schedule->OnLoadingCompleted ();
}

// --------------------------------------------------------------------------------
Contest::~Contest ()
{
  _state = LEAVING;

  // www
#ifndef DEBUG
  if (_filename)
  {
    gchar *base_name = g_path_get_basename (_filename);
    gchar *www_file  = g_build_filename (Global::_share_dir, "webserver", "LightTPD", "www", "cotcot", base_name, NULL);

    g_unlink (www_file);

    g_free (base_name);
    g_free (www_file);
  }
#endif

  g_free (_authority);
  g_free (_fie_id);
  g_free (_name);
  g_free (_filename);
  g_free (_organizer);
  g_free (_source);
  g_free (_web_site);
  g_free (_location);

  Object::TryToRelease (_manual_classification);
  Object::TryToRelease (_minimum_team_size);
  Object::TryToRelease (_default_classification);

  Object::TryToRelease (_checkin_time);
  Object::TryToRelease (_scratch_time);
  Object::TryToRelease (_start_time);

  gdk_color_free (_gdk_color);

  Object::TryToRelease (_schedule);
  Object::TryToRelease (_referees_list);
  Object::TryToRelease (_category);

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

  parcel->Set ("color",    color);
  parcel->Set ("weapon",   _weapon->GetXmlImage ());
  parcel->Set ("gender",   gender_xml_image[_gender]);
  parcel->Set ("category", _category->GetXmlImage ());

  g_free (color);
}

// --------------------------------------------------------------------------------
gchar *Contest::GetFilename ()
{
  return _filename;
}

// --------------------------------------------------------------------------------
void Contest::SetFilename (gchar *filename)
{
  g_free (_filename);
  _filename = filename;

  if (filename)
  {
    gchar *base_name = g_path_get_basename (_filename);
    gchar *suffix    = g_strrstr (base_name,
                                  ".cotcot");
    if (suffix)
    {
      *suffix = '\0';

      g_list_foreach (_advertisers,
                      (GFunc) Net::Advertiser::SetTitle,
                      base_name);
    }

    g_free (base_name);
  }
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
void Contest::AskForSettings ()
{
  _schedule->SetScoreStuffingPolicy (FALSE);

  FillInProperties ();
  _schedule->CreateDefault ();

  _schedule->SetTeamEvent (FALSE);

  {
    RunDialog (GTK_DIALOG (_properties_dialog));
    ReadProperties ();
    gtk_widget_hide (_properties_dialog);
  }

  if (_read_only == FALSE)
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
Contest *Contest::Duplicate ()
{
  Contest *contest = new Contest (_advertisers, TRUE);

  contest->_schedule->CreateDefault (TRUE);
  contest->_derived = TRUE;

  if (_authority)
  {
    contest->_authority = g_strdup (_authority);
  }
  if (_fie_id)
  {
    g_free (contest->_fie_id);
    contest->_fie_id = g_strdup (_fie_id);
  }
  contest->_name       = g_strdup (_name);
  contest->_organizer  = g_strdup (_organizer);
  contest->_source     = g_strdup (_source);
  contest->_web_site   = g_strdup (_web_site);
  contest->_location   = g_strdup (_location);
  contest->_category->Replicate (_category);
  contest->_level      = _level;
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
  return _referees_list->GetPlayerFromRef (ref);
}

// --------------------------------------------------------------------------------
void Contest::ManageReferee (Net::Message *message)
{
  gchar     *xml = message->GetString ("xml");
  xmlDocPtr  doc = xmlParseMemory (xml, strlen (xml));

  if (doc)
  {
    xmlXPathInit ();

    {
      xmlXPathContext *xml_context = xmlXPathNewContext (doc);
      xmlXPathObject  *xml_object;
      xmlNodeSet      *xml_nodeset;

      xml_object = xmlXPathEval (BAD_CAST "/Arbitre", xml_context);
      xml_nodeset = xml_object->nodesetval;

      if (xml_nodeset->nodeNr == 1)
      {
        Player::AttributeId weapon_attr_id ("weapon");
        Attribute *weapon_attr;
        Player    *referee     = PlayerFactory::CreatePlayer ("Referee");

        referee->Load (xml_nodeset->nodeTab[0]);

        weapon_attr = referee->GetAttribute (&weapon_attr_id);
        if (   weapon_attr
            && (g_ascii_strcasecmp (_weapon->GetXmlImage (), weapon_attr->GetStrValue ()) == 0))
        {
          Player *old = _referees_list->GetPlayerFromRef (referee->GetRef ());

          if (old)
          {
            old->UpdateFrom (referee);
            old->RefreshParcel ();
          }
          else
          {
            _referees_list->Add (referee);
          }
        }

        referee->Release ();
      }

      xmlXPathFreeObject  (xml_object);
      xmlXPathFreeContext (xml_context);
    }
    xmlFreeDoc (doc);
  }

  g_free (xml);
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

  gtk_combo_box_set_active (GTK_COMBO_BOX (_level_combo),
                            _level);

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
  _level    = gtk_combo_box_get_active (GTK_COMBO_BOX (_level_combo));

  _category->ReadUserChoice ();

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
                        _category->GetDisplayImage ());
  }

  w = _glade->GetWidget ("contest_level_label");
  if (w)
  {
    gtk_label_set_text (GTK_LABEL (w),
                        gettext (level_image[_level]));
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
  if (_tournament)
  {
    Publication       *publication = _tournament->GetPublication ();
    Net::FileUploader *uploader    = publication->GetUpLoader ();

    if (uploader)
    {
      Net::GregUploader *greg_uploader = dynamic_cast <Net::GregUploader *> (uploader);

      if (greg_uploader)
      {
        greg_uploader->SetTeamEvent    (_team_event);
        greg_uploader->SetDate         (_day, _month, _year);
        greg_uploader->SetLocation     (_location);
        greg_uploader->SetLevel        (level_xml_image[_level]);
        greg_uploader->SetDivision     ("");
        greg_uploader->SetRunningState (_schedule->IsOver ());
        greg_uploader->SetWeapon       (_weapon->GetGregImage ());
        greg_uploader->SetGender       (_gender);
        greg_uploader->SetCategory     (_category->GetXmlImage ());
      }

      g_list_foreach (_advertisers,
                      (GFunc) Net::Advertiser::SetLink,
                      (gpointer) uploader->GetWWW ());

      uploader->UploadFile (_filename);
      uploader->Release ();
    }
  }
}

// --------------------------------------------------------------------------------
void Contest::Save ()
{
  if (_filename)
  {
    // Regular
    Save (_filename);

    // www
#ifndef DEBUG
    {
      gchar *base_name = g_path_get_basename (_filename);
      gchar *www_file  = g_build_filename (Global::_share_dir, "webserver", "LightTPD", "www", "cotcot", base_name, NULL);

      Save (www_file);
      g_free (base_name);
      g_free (www_file);
    }
#endif

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

    if (_authority)
    {
      xmlTextWriterWriteAttribute (xml_writer,
                                   BAD_CAST "Championnat",
                                   BAD_CAST _authority);
    }

    if (_fie_id)
    {
      xmlTextWriterWriteAttribute (xml_writer,
                                   BAD_CAST "ID",
                                   BAD_CAST _fie_id);
    }

    xmlTextWriterWriteFormatAttribute (xml_writer,
                                       BAD_CAST "NetID",
                                       "%x", GetNetID ());

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
    if (_source)
    {
      xmlTextWriterWriteAttribute (xml_writer,
                                   BAD_CAST "Source",
                                   BAD_CAST _source);
    }
    xmlTextWriterWriteAttribute (xml_writer,
                                 BAD_CAST "Categorie",
                                 BAD_CAST _category->GetXmlImage ());
    xmlTextWriterWriteAttribute (xml_writer,
                                 BAD_CAST "Niveau",
                                 BAD_CAST level_xml_image[_level]);
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

  response = RunDialog (GTK_DIALOG (chooser));

  if (response == GTK_RESPONSE_ACCEPT)
  {
    SetFilename (gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser)));

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

      AddFileToRecentManager (_filename);
    }
  }

  if (choice)
  {
    *choice = response;
  }

  return _filename;
}

// --------------------------------------------------------------------------------
void Contest::AddFileToRecentManager (const gchar *filename)
{
  gchar *uri = g_filename_to_uri (filename,
                                  NULL,
                                  NULL);

  gtk_recent_manager_add_item (gtk_recent_manager_get_default (),
                               uri);
  g_free (uri);
}

// --------------------------------------------------------------------------------
gboolean Contest::OnMessage (Net::Message *message)
{
  return _schedule->OnMessage (message);
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
void Contest::TweetFeeder (Net::Advertiser::Feeder *feeder)
{
  if (_state == OPERATIONAL)
  {
    g_list_foreach (_advertisers,
                    (GFunc) Net::Advertiser::TweetFeeder,
                    feeder);
  }
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

  {
    gchar *color = gdk_color_to_string (_gdk_color);

    goo_canvas_rect_new (goo_canvas_get_root_item (canvas),
                         0.0, 0.0,
                         100.0, PRINT_HEADER_FRAME_HEIGHT,
                         "stroke-color", "grey",
                         "fill-color", color,
                         "line-width", 0.3,
                         NULL);

    g_free (color);
  }

  goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                       _name,
                       10.0, 0.8,
                       20.0,
                       GTK_ANCHOR_NORTH,
                       "alignment", PANGO_ALIGN_CENTER,
                       "fill-color", "black",
                       "font", BP_FONT "Bold 2.5px", NULL);

  {
    char *text = g_strdup_printf ("%s - %s - %s", _weapon->GetImage (),
                                  gettext (gender_image[_gender]),
                                  _category->GetDisplayImage ());
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

  goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                       (const char *) g_object_get_data (G_OBJECT (operation), "Print::PageName"),
                       50.0, 7.5,
                       -1.0,
                       GTK_ANCHOR_CENTER,
                       "fill-color", "black",
                       "font", BP_FONT "Bold 4px", NULL);

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

  goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                       GetDate (),
                       98.0, 5.0,
                       -1.0,
                       GTK_ANCHOR_EAST,
                       "fill-color", "black",
                       "font", BP_FONT "Bold 2.5px", NULL);

  goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                       "BellePoule fencing software "
                       "<span foreground=\"blue\" underline=\"single\">http://betton.escrime.free.fr</span> ",
                       50.0, 145.5,
                       100.0,
                       GTK_ANCHOR_S,
                       "alignment",  PANGO_ALIGN_CENTER,
                       "fill-color", "black",
                       "font",       BP_FONT "Bold 1.7px",
                       "use-markup", TRUE,
                       NULL);

  goo_canvas_render (canvas,
                     gtk_print_context_get_cairo_context (context),
                     NULL,
                     1.0);

  gtk_widget_destroy (GTK_WIDGET (canvas));

  if (operation_matrix == NULL)
  {
    cairo_t *cr = gtk_print_context_get_cairo_context (context);

    cairo_translate (cr,
                     0.0,
                     GetPrintHeaderSize (context, ON_SHEET));
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
  RunDialog (GTK_DIALOG (_properties_dialog));
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

  if (RunDialog (GTK_DIALOG (dialog)) == TRUE)
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
  gint choice = GTK_RESPONSE_ACCEPT;

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
guint Contest::GetNetID ()
{
  return _parcel->GetNetID ();
}

// --------------------------------------------------------------------------------
gchar *Contest::GetDefaultFileName ()
{
  return g_strdup_printf ("%s-%s-%s-%s.cotcot", _name,
                          _weapon->GetImage (),
                          gettext (gender_image[_gender]),
                          _category->GetDisplayImage ());
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
const gchar *Contest::GetCategory ()
{
  return _category->GetDisplayImage ();
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
