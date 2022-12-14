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
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>
#include <goocanvas.h>

#include "util/global.hpp"
#include "util/canvas.hpp"
#include "util/user_config.hpp"
#include "util/attribute.hpp"
#include "util/attribute_desc.hpp"
#include "util/filter.hpp"
#include "util/glade.hpp"
#include "util/data.hpp"
#include "util/flash_code.hpp"
#include "util/xml_scheme.hpp"
#include "actors/player_factory.hpp"
#include "network/greg_uploader.hpp"
#include "network/message.hpp"
#include "actors/checkin.hpp"
#include "actors/referees_list.hpp"
#include "actors/player_factory.hpp"
#include "actors/team.hpp"
#include "rounds/checkin/checkin_supervisor.hpp"
#include "publication.hpp"
#include "schedule.hpp"
#include "zipper.hpp"

#include "application/version.h"
#include "application/weapon.hpp"
#include "category.hpp"

#include "askfred/reader.hpp"
#include "askfred/writer.hpp"

#include "tournament.hpp"
#include "contest.hpp"

const gchar *Contest::gender_image[_nb_gender] =
{
  N_ ("Men"),
  N_ ("Women"),
  N_ ("Mixed")
};

const gchar *Contest::gender_xml_image[_nb_gender] =
{
  "M",
  "F",
  "X"
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

GList *Contest::_color_list = nullptr;

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
void Contest::Time::Save (XmlScheme   *xml_scheme,
                          const gchar *attr_name)
{
  xml_scheme->WriteFormatAttribute (attr_name,
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
  Disclose ("BellePoule::Competition");

  _save_timeout_id = 0;

  _fie_id = g_strdup_printf ("%x", GetNetID ());

  _read_only   = FALSE;
  _authority   = nullptr;
  _filename    = nullptr;
  _tournament  = nullptr;
  _weapon      = Weapon::GetDefault ();
  _category    = new Category (_glade->GetWidget ("category_combobox"));
  _level       = 0;
  _gender      = 0;
  _elo_matters = FALSE;
  _team_event  = FALSE;
  _derived     = FALSE;
  _source      = nullptr;
  _advertisers = advertisers;
  _locked      = FALSE;

  _name = g_key_file_get_string (Global::_user_config->_key_file,
                                 "Competiton",
                                 "default_name",
                                 nullptr);
  _organizer = g_key_file_get_string (Global::_user_config->_key_file,
                                      "Competiton",
                                      "default_organizer",
                                      nullptr);
  _web_site = g_key_file_get_string (Global::_user_config->_key_file,
                                     "Competiton",
                                     "default_web_site",
                                     nullptr);
  _location = g_key_file_get_string (Global::_user_config->_key_file,
                                     "Competiton",
                                     "default_location",
                                     nullptr);
  _label = g_strdup ("");

  _manual_classification  = new Data ("ClassementManuel",       (guint) (for_duplication == TRUE));
  _minimum_team_size      = new Data ("TailleMinimaleEquipe",   3);
  _default_classification = new Data ("ClassementDefautEquipe", 9999);

  _state = State::OPERATIONAL;

  _checkin_time = new Time ("checkin");
  _scratch_time = new Time ("scratch");
  _start_time   = new Time ("start");

  AddSensitiveWidget (_glade->GetWidget ("team_vbox"));
  AddSensitiveWidget (_glade->GetWidget ("elo_frame"));

  {
    GtkComboBox  *combo = GTK_COMBO_BOX (_glade->GetWidget ("color_combobox"));
    GtkTreeModel *model = gtk_combo_box_get_model (combo);

    for (GList *current = _color_list; current; current = g_list_next (current))
    {
      GtkTreeIter iter;

      gtk_list_store_append (GTK_LIST_STORE (model),
                             &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          0, gdk_color_to_string  ((GdkColor *) current->data),
                          -1);
    }
  }

  ChooseColor ();

  _properties_dialog = _glade->GetWidget ("properties_dialog");

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
    GtkTreeModel *model;

    _weapon_combo = _glade->GetWidget ("weapon_combo");
    model         = gtk_combo_box_get_model (GTK_COMBO_BOX (_weapon_combo));

    for (GList *current = Weapon::GetList (); current; current = g_list_next (current))
    {
      GtkTreeIter  iter;
      Weapon      *weapon = (Weapon *) current->data;

      gtk_list_store_append (GTK_LIST_STORE (model),
                             &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          0, weapon->GetImage (),
                          1, TRUE,
                          2, weapon,
                          -1);
    }

    g_signal_connect (G_OBJECT (_weapon_combo), "changed",
                      G_CALLBACK (OnWeaponChanged),
                      this);
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
        if (AttributeDesc::GetDescFromXmlName ((const gchar *) current_attr->name) == nullptr)
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
void Contest::LoadAskFred (AskFred::Reader::Event *askfred,
                           const gchar            *dirname)
{
  g_free (_name);
  _name = g_strdup (askfred->_name);

  g_free (_location);
  _location = g_strdup (askfred->_location);

  g_free (_organizer);
  _organizer = g_strdup (askfred->_organizer);

  g_free (_fie_id);
  _fie_id = g_strdup (askfred->_event_id);

  g_free (_source);
  _source = g_strdup_printf ("AskFred:%s", askfred->_tournament_id);

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

  _team_event = askfred->_team_event;

  if (_schedule)
  {
    AttributeDesc             *league_desc = AttributeDesc::GetDescFromCodeName ("league");
    People::CheckinSupervisor *checkin;
    GList                     *current  = askfred->_competitors;

    league_desc->_favorite_look = AttributeDesc::SHORT_TEXT;
    _schedule->CreateDefault ();
    _schedule->SetTeamEvent (_team_event);
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
      AskFred::Reader::Competitor *competitor = (AskFred::Reader::Competitor *) current->data;
      Player                      *player     = competitor->CreatePlayer (askfred->_weapon);

      checkin->Add (player);

      if (player->Is ("Team"))
      {
        Team   *team    = (Team *) player;
        GSList *members = team->GetMemberList ();

        while (members)
        {
          Player *member = (Player *) members->data;

          checkin->Add (member);
          member->Release ();

          members = g_slist_next (members);
        }
      }

      player->Release ();

      current = g_list_next (current);
    }
  }

  FillInProperties ();
}

// --------------------------------------------------------------------------------
void Contest::LoadXmlString (const guchar *string)
{
  {
    xmlDoc *doc = xmlParseDoc (string);

    if (doc)
    {
      LoadXmlDocument (doc);

      if (_save_timeout_id > 0)
      {
        g_source_remove (_save_timeout_id);
      }

      xmlFreeDoc (doc);
    }
  }

  if ((_filename == nullptr) && (_read_only == FALSE))
  {
    GtkWidget *chooser = GTK_WIDGET (gtk_file_chooser_dialog_new (gettext ("Choose a file..."),
                                                                  nullptr,
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
void Contest::LoadXmlFile (const gchar *filename)
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
      LoadXmlDocument (doc);

      AddFileToRecentManager (filename);

      if (g_str_has_suffix (_filename,
                            ".cotcot") == FALSE)
      {
        SetFilename (nullptr);
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

  if ((_filename == nullptr) && (_read_only == FALSE))
  {
    GtkWidget *chooser = GTK_WIDGET (gtk_file_chooser_dialog_new (gettext ("Choose a file..."),
                                                                  nullptr,
                                                                  GTK_FILE_CHOOSER_ACTION_SAVE,
                                                                  GTK_STOCK_CANCEL,  GTK_RESPONSE_CANCEL,
                                                                  GTK_STOCK_SAVE_AS, GTK_RESPONSE_ACCEPT,
                                                                  NULL));

    GetSaveFileName (chooser,
                     "default_dir_name");
    gtk_widget_destroy (chooser);

    Save ();
  }
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
void Contest::LoadXmlDocument (xmlDoc *doc)
{
  _state = State::LOADING;

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
            _gdk_color = nullptr;
          }
          xmlFree (attr);
        }
        if (_gdk_color == nullptr)
        {
          ChooseColor ();
        }
        else
        {
          PaintColor ();
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
                                               nullptr,
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
        if (attr == nullptr)
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

          if (_weapon->GetXmlImage ()[0] == 'L')
          {
            DisableSensitiveWidgets ();
          }
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

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "Label");
        if (attr)
        {
          g_free (_label);
          _label = g_strdup (attr);
          xmlFree (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "ScoreAleatoire");
        if (attr)
        {
          score_stuffing_policy = (gboolean) atoi (attr);
          xmlFree (attr);
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "EloComptePourClassement");
        if (attr)
        {
          _elo_matters = (gboolean) atoi (attr);
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

      if (_source && (g_strstr_len (_source, -1, "AskFred") == _source))
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

    if (_source && (g_strstr_len (_source, -1, "AskFred") == _source))
    {
      People::CheckinSupervisor *checkin = _schedule->GetCheckinSupervisor ();
      Filter                    *filter  = checkin->GetFilter ();

      filter->ShowAttribute ("rating");
      filter->UpdateAttrList (FALSE);
      filter->Release ();
    }
  }

  FillInProperties ();

  _state = State::OPERATIONAL;
  _schedule->OnLoadingCompleted ();
}

// --------------------------------------------------------------------------------
Contest::~Contest ()
{
  Recall ();

  _state = State::LEAVING;

  // www
  if (_filename && Global::_www)
  {
    gchar *base_name = g_path_get_basename (_filename);
    gchar *www_file  = g_build_filename (Global::_www, base_name, NULL);

    g_unlink (www_file);

    g_free (base_name);
    g_free (www_file);
  }

  g_free (_authority);
  g_free (_fie_id);
  g_free (_name);
  g_free (_filename);
  g_free (_organizer);
  g_free (_source);
  g_free (_web_site);
  g_free (_location);
  g_free (_label);

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
                                                                  nullptr,
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
Contest *Contest::Duplicate (const gchar *label)
{
  Contest *contest = new Contest (_advertisers, TRUE);

  contest->_schedule->CreateDefault (TRUE);
  contest->_derived = TRUE;

  if (label)
  {
    g_free (contest->_label);
    contest->_label = g_strdup (label);
  }
  if (_authority)
  {
    contest->_authority = g_strdup (_authority);
  }
  if (_fie_id)
  {
    g_free (contest->_fie_id);
    contest->_fie_id = g_strdup (_fie_id);
  }
  contest->_name        = g_strdup (_name);
  contest->_organizer   = g_strdup (_organizer);
  contest->_source      = g_strdup (_source);
  contest->_web_site    = g_strdup (_web_site);
  contest->_location    = g_strdup (_location);
  contest->_category->Replicate (_category);
  contest->_level       = _level;
  contest->_weapon      = _weapon;
  contest->_gender      = _gender;
  contest->_day         = _day;
  contest->_month       = _month;
  contest->_year        = _year;
  contest->_tournament  = _tournament;
  contest->_team_event  = _team_event;
  contest->_elo_matters = _elo_matters;
  contest->_schedule->SetTeamEvent (_team_event);

  _checkin_time->Copy (contest->_checkin_time);
  _scratch_time->Copy (contest->_scratch_time);
  _start_time->Copy   (contest->_start_time);

  contest->_minimum_team_size->Copy (_minimum_team_size);

  contest->FillInProperties ();

  for (gboolean ready_for_greg = FALSE; ready_for_greg == FALSE;)
  {
    gtk_dialog_run (GTK_DIALOG (contest->_properties_dialog));
    contest->ReadProperties ();
    gtk_widget_hide (contest->_properties_dialog);

    if (contest->_label[0] == '\0')
    {
      GtkWidget *dialog = gtk_message_dialog_new (NULL,
                                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_MESSAGE_WARNING,
                                                  GTK_BUTTONS_YES_NO,
                                                  gettext ("GREG alert"));

      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                                gettext ("It's recommended to give a label when publication on GREG (www.escrime-info.com) is intended.\n\n"
                                                         "Re-open the form to fill it in?"));

      if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_YES)
      {
        ready_for_greg = TRUE;
      }
      gtk_widget_destroy (dialog);
    }
    else
    {
      ready_for_greg = TRUE;
    }
  }

  return contest;
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
        if (weapon_attr)
        {
          gchar *upper = g_ascii_strup (weapon_attr->GetStrValue (),
                                        -1);

          if (g_strrstr (upper, _weapon->GetXmlImage ()))
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

              {
                Net::Message *parcel = referee->Disclose ("BellePoule::Referee");

                referee->FeedParcel (parcel);
                Net::Ring::_broker->InjectMessage (parcel,
                                                   this->GetParcel ());
              }
            }

            MakeDirty ();
          }
          g_free (upper);
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
                                         nullptr);
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

  PaintColor ();
}

// --------------------------------------------------------------------------------
void Contest::PaintColor ()
{
  if (_gdk_color)
  {
    {
      GtkWidget *tab      = _glade->GetWidget ("eventbox");
      GtkWidget *colorbox = _glade->GetWidget ("colorbox");

      gtk_widget_modify_bg (tab,
                            GTK_STATE_NORMAL,
                            _gdk_color);
      gtk_widget_modify_bg (tab,
                            GTK_STATE_ACTIVE,
                            _gdk_color);

      gtk_widget_modify_bg (colorbox,
                            GTK_STATE_NORMAL,
                            _gdk_color);
      gtk_widget_modify_bg (colorbox,
                            GTK_STATE_ACTIVE,
                            _gdk_color);
    }

    {
      GtkComboBox  *combo         = GTK_COMBO_BOX (_glade->GetWidget ("color_combobox"));
      GtkTreeModel *model         = gtk_combo_box_get_model (combo);
      GtkTreeIter   iter;
      gboolean      iter_is_valid;
      gboolean      not_found = TRUE;

      iter_is_valid = gtk_tree_model_get_iter_first (model,
                                                     &iter);

      while (iter_is_valid && not_found)
      {
        gchar    *color_name;
        GdkColor  color;

        gtk_tree_model_get (model, &iter,
                            0, &color_name, -1);
        if (gdk_color_parse (color_name,
                             &color))
        {
          if (gdk_color_equal (_gdk_color,
                               &color))
          {
            gtk_combo_box_set_active_iter (combo,
                                           &iter);
            not_found = FALSE;
          }
        }
        g_free (color_name);

        iter_is_valid = gtk_tree_model_iter_next (model,
                                                  &iter);
      }
    }
  }
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
void Contest::Lock ()
{
  if (_locked == FALSE)
  {
    DisableSensitiveWidgets ();

    {
      GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (_weapon_combo));
      gboolean      iter_is_valid;
      GtkTreeIter   iter;

      iter_is_valid = gtk_tree_model_get_iter_first (model,
                                                     &iter);

      while (iter_is_valid)
      {
        Weapon *weapon;

        gtk_tree_model_get (model, &iter,
                            2, &weapon,
                            -1);

        gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                            1, _weapon->HasSameAffinity (weapon),
                            -1);

        iter_is_valid = gtk_tree_model_iter_next (model,
                                                  &iter);
      }
    }

    _locked = TRUE;
  }
}

// --------------------------------------------------------------------------------
void Contest::UnLock ()
{
  if (_locked)
  {
    EnableSensitiveWidgets ();

    {
      GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (_weapon_combo));
      gboolean      iter_is_valid;
      GtkTreeIter   iter;

      iter_is_valid = gtk_tree_model_get_iter_first (model,
                                                     &iter);

      while (iter_is_valid)
      {
        gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                            1, TRUE,
                            -1);

        iter_is_valid = gtk_tree_model_iter_next (model,
                                                  &iter);
      }
    }

    _locked = FALSE;
  }
}

// --------------------------------------------------------------------------------
void Contest::ReloadFencers ()
{
  People::CheckinSupervisor *checkin = _schedule->GetCheckinSupervisor ();

  checkin->ReloadFencers ();
}

// --------------------------------------------------------------------------------
gboolean Contest::EloMatters ()
{
  return _elo_matters;
}

// --------------------------------------------------------------------------------
gboolean Contest::ConfigValidated ()
{
  if ((_weapon->GetXmlImage ()[0] != 'L') && _elo_matters)
  {
    gint       response;
    GtkWidget *w = _glade->GetWidget ("elo_dialog");

    response = RunDialog (GTK_DIALOG (w));
    gtk_widget_hide (w);

    return response == GTK_RESPONSE_YES;
  }

  return TRUE;
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
                  nullptr);
  g_list_free (_color_list);

  _color_list = nullptr;
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

  gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (_glade->GetWidget ("label_entry")))),
                      _label);

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

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("elo_checkbutton")),
                                (_elo_matters == TRUE));

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

  entry = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (_glade->GetWidget ("label_entry"))));
  str = (gchar *) gtk_entry_get_text (entry);
  if (str[0] != '\0')
  {
    str = g_strstrip (str);
  }
  g_free (_label);
  _label = g_strdup (str);

  _weapon = Weapon::GetFromIndex (gtk_combo_box_get_active (GTK_COMBO_BOX (_weapon_combo)));
  _gender = gtk_combo_box_get_active (GTK_COMBO_BOX (_gender_combo));
  _level  = gtk_combo_box_get_active (GTK_COMBO_BOX (_level_combo));

  _elo_matters = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("elo_checkbutton")));


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

  gtk_label_set_text  (GTK_LABEL (_glade->GetWidget ("banner")),
                       "");

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
void Contest::OnColorChanged (GtkComboBox *widget)
{
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (widget,
                                     &iter))
  {
    GtkTreeModel *model      = gtk_combo_box_get_model (widget);
    gchar        *color_name;
    GdkColor      color;

    gtk_tree_model_get (model, &iter,
                        0, &color_name, -1);
    if (gdk_color_parse (color_name,
                         &color))
    {
      if (gdk_color_equal (_gdk_color,
                           &color) == FALSE)
      {
        gdk_color_free (_gdk_color);
        _gdk_color = gdk_color_copy (&color);

        PaintColor ();
      }
    }
    g_free (color_name);
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
        greg_uploader->SetLocation     (_location, _label);
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
void Contest::Archive (const gchar *title)
{
  if (_state == State::OPERATIONAL)
  {
    Save ();

    if (_filename)
    {
      gchar *zip_name = g_strdup (_filename);
      gchar *suffix   = g_strrstr (zip_name, ".cotcot");

      if (suffix)
      {
        g_sprintf (suffix, ".zip");

        {
          Zipper *zipper = new Zipper (zip_name);

          zipper->Archive2 (_filename, title);
          zipper->Release ();
        }
      }
      g_free (zip_name);
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
    if (Global::_www)
    {
      gchar *base_name = g_path_get_basename (_filename);
      gchar *www_file  = g_build_filename (Global::_www, base_name, NULL);

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
void Contest::SaveHeader (XmlScheme *xml_scheme)
{
  {
    if (_team_event)
    {
      xml_scheme->StartDTD ("CompetitionParEquipes");
    }
    else
    {
      xml_scheme->StartDTD ("CompetitionIndividuelle");
    }
    xml_scheme->EndDTD ();

    xml_scheme->StartComment ();
    xml_scheme->WriteFormatString ("\n");
    xml_scheme->WriteFormatString ("           By BellePoule (V%s.%s%s)\n", VERSION, VERSION_REVISION, VERSION_MATURITY);
    xml_scheme->WriteFormatString ("\n");
    xml_scheme->WriteFormatString ("   http://betton.escrime.free.fr/index.php/bellepoule\n");
    xml_scheme->EndComment ();

    if (_team_event)
    {
      xml_scheme->StartElement ("CompetitionParEquipes");
    }
    else
    {
      xml_scheme->StartElement ("CompetitionIndividuelle");
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
        xml_scheme->WriteAttribute ("Couleur",
                                    color);
        g_free (color);
      }
    }

    if (_authority)
    {
      xml_scheme->WriteAttribute ("Championnat",
                                  _authority);
    }

    if (_fie_id)
    {
      xml_scheme->WriteAttribute ("ID",
                                  _fie_id);
    }

    xml_scheme->WriteFormatAttribute ("NetID",
                                      "%x", GetNetID ());

    xml_scheme->WriteFormatAttribute ("Annee",
                                      "%d", _year);
    xml_scheme->WriteAttribute ("Arme",
                                _weapon->GetXmlImage ());
    xml_scheme->WriteAttribute ("Sexe",
                                gender_xml_image[_gender]);
    xml_scheme->WriteAttribute ("Organisateur",
                                _organizer);
    if (_source)
    {
      xml_scheme->WriteAttribute ("Source",
                                  _source);
    }
    xml_scheme->WriteAttribute ("Categorie",
                                _category->GetXmlImage ());
    xml_scheme->WriteAttribute ("Niveau",
                                level_xml_image[_level]);
    xml_scheme->WriteFormatAttribute ("Date",
                                      "%02d.%02d.%d", _day, _month, _year);
    if (_checkin_time)
    {
      _checkin_time->Save (xml_scheme,
                           "Appel");
    }
    if (_scratch_time)
    {
      _scratch_time->Save (xml_scheme,
                           "Scratch");
    }
    if (_start_time)
    {
      _start_time->Save   (xml_scheme,
                           "Debut");
    }
    xml_scheme->WriteAttribute ("TitreLong",
                                _name);
    xml_scheme->WriteAttribute ("URLorganisateur",
                                _web_site);
    xml_scheme->WriteFormatAttribute ("ScoreAleatoire",
                                      "%d", _schedule->ScoreStuffingIsAllowed ());
    xml_scheme->WriteFormatAttribute ("EloComptePourClassement",
                                      "%d", _elo_matters);
    // Team configuration
    {
      _minimum_team_size->Save      (xml_scheme);
      _default_classification->Save (xml_scheme);
      _manual_classification->Save  (xml_scheme);
    }

    xml_scheme->WriteAttribute ("Lieu",
                                _location);
    xml_scheme->WriteAttribute ("Label",
                                _label);
  }

  _schedule->SavePeoples (xml_scheme,
                          _referees_list);
}

// --------------------------------------------------------------------------------
void Contest::Save (gchar *filename)
{
  if (filename)
  {
    {
      XmlScheme *xml_scheme = new XmlScheme (filename);

      SaveHeader (xml_scheme);
      _schedule->Save (xml_scheme);

      xml_scheme->EndElement (); // toto

      xml_scheme->Release ();
    }

    gtk_widget_set_sensitive (_glade->GetWidget ("save_toolbutton"),
                              FALSE);
  }
}

// --------------------------------------------------------------------------------
void Contest::DumpToFRD (gchar *filename)
{
  if (filename)
  {
    XmlScheme *xml_scheme = new AskFred::Writer::Scheme (filename);

    // FencingData
    {
      xml_scheme->StartElement ("FencingData");

      xml_scheme->WriteAttribute ("Version",
                                  "5.0");
      xml_scheme->WriteAttribute ("Context",
                                  "Results");
      xml_scheme->WriteAttribute ("Source",
                                  "BellePouleV5.0");
      xml_scheme->WriteAttribute ("xmlns",
                                  "http://www.askfred.net");

      // Tournament
      {
        xml_scheme->StartElement ("Tournament");

        {
          const gchar *id = g_strrstr (_source, ":");

          if (id)
          {
            xml_scheme->WriteAttribute ("TournamentID",
                                        (id + 1));
          }
          xml_scheme->WriteAttribute ("Name",
                                      _name);
          xml_scheme->WriteAttribute ("Location",
                                      _location);
        }

        // Event
        {
          xml_scheme->StartElement ("Event");

          xml_scheme->WriteAttribute ("EventID",
                                      _fie_id);
          xml_scheme->WriteAttribute ("Weapon",
                                      _weapon->GetImage ());
          xml_scheme->WriteAttribute ("Gender",
                                      gender_xml_image[_gender]);

          if (_team_event)
          {
            xml_scheme->WriteAttribute ("IsTeam",
                                        "True");
          }
          else
          {
            xml_scheme->WriteAttribute ("IsTeam",
                                        "False");
          }

          _schedule->SaveFinalResults (xml_scheme);

          _schedule->Save (xml_scheme);

          xml_scheme->EndElement ();
        }

        xml_scheme->EndElement ();
      }

      // Databases
      {
        _schedule->SavePeoples (xml_scheme,
                                _referees_list);

        xml_scheme->WriteCustom ("ClubDatabase");
      }

      xml_scheme->EndElement ();
    }

    xml_scheme->Release ();
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
                                                 nullptr);
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
                                  nullptr,
                                  nullptr);

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
void Contest::TweetFeeder (Net::Advertiser::Feeder *feeder)
{
  if (_state == State::OPERATIONAL)
  {
    g_list_foreach (_advertisers,
                    (GFunc) Net::Advertiser::TweetFeeder,
                    feeder);
  }
}

// --------------------------------------------------------------------------------
void Contest::OnWeaponChanged (GtkComboBox *combobox,
                               Contest      *contest)
{
  Weapon *weapon = Weapon::GetFromIndex (gtk_combo_box_get_active (combobox));

  if (weapon->GetXmlImage ()[0] == 'L')
  {
    if (weapon->HasSameAffinity (contest->_weapon) == FALSE)
    {
      contest->DisableSensitiveWidgets ();
    }

    contest->_schedule->Mutate ("TourDePoules",
                                "RondeSuisse");
    contest->_schedule->Mutate ("PhaseDeTableaux",
                                "TableauSuisse");
  }
  else
  {
    if (weapon->HasSameAffinity (contest->_weapon) == FALSE)
    {
      contest->EnableSensitiveWidgets ();
    }

    contest->_schedule->Mutate ("RondeSuisse",
                                "TourDePoules");
    contest->_schedule->Mutate ("TableauSuisse",
                                "PhaseDeTableaux");
  }

  contest->_weapon = weapon;
}

// --------------------------------------------------------------------------------
void Contest::on_web_site_button_clicked ()
{
  GtkWidget *entry = _glade->GetWidget ("web_site_entry");

  ShowUri (gtk_entry_get_text (GTK_ENTRY (entry)));
}

// --------------------------------------------------------------------------------
void Contest::DrawConfig (GtkPrintOperation *operation,
                          GtkPrintContext   *context,
                          gint               page_nr)
{
  DrawConfigLine (operation,
                  context,
                  "Coefficient K du classement ELO \"Hauts de France\" : 50");
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
                       "alignment",  PANGO_ALIGN_CENTER,
                       "fill-color", "black",
                       "font",       BP_FONT "Bold 2.5px",
                       "height",     9.0,
                       "wrap",       PANGO_WRAP_WORD_CHAR,
                       NULL);

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
                       "font", BP_FONT "Bold 3px", NULL);

  if (_organizer)
  {
    goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                         _organizer,
                         99.0, 4.0,
                         20.0,
                         GTK_ANCHOR_EAST,
                         "fill-color", "black",
                         "font",       BP_FONT "Bold 2.5px",
                         "height",     5.5,
                         "wrap",       PANGO_WRAP_WORD_CHAR,
                         "alignment",  PANGO_ALIGN_RIGHT,
                         NULL);
  }

  goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                       GetDate (),
                       99.0, 8.0,
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
                     nullptr,
                     1.0);

  gtk_widget_destroy (GTK_WIDGET (canvas));

  if (operation_matrix == nullptr)
  {
    cairo_t *cr = gtk_print_context_get_cairo_context (context);

    cairo_translate (cr,
                     0.0,
                     GetPrintHeaderSize (context, SizeReferential::ON_SHEET));
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
  if ((_filename == nullptr) && (_read_only == FALSE))
  {
    GtkWidget *chooser = GTK_WIDGET (gtk_file_chooser_dialog_new (gettext ("Choose a file..."),
                                                                  nullptr,
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
  gtk_label_set_text  (GTK_LABEL (_glade->GetWidget ("banner")),
                       "");

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

  if ((_filename == nullptr) && (_read_only == FALSE))
  {
    GtkWidget *chooser = GTK_WIDGET (gtk_file_chooser_dialog_new (gettext ("Choose a file..."),
                                                                  nullptr,
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
gboolean Contest::HasAskFredEntry ()
{
  return _source != nullptr;
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
  gchar   *filename;
  GString *string = g_string_new (nullptr);

  string = g_string_append (string, _name);

  string = g_string_append_c (string, '-');
  string = g_string_append (string, gettext (_weapon->GetImage ()));

  string = g_string_append_c (string, '-');
  string = g_string_append (string, gettext (gender_image[_gender]));

  string = g_string_append_c (string, '-');
  string = g_string_append (string, _category->GetDisplayImage ());

  if (_label && (_label[0] != '\0'))
  {
    string = g_string_append_c (string, '-');
    string = g_string_append (string, _label);
  }

  string = g_string_append (string, ".cotcot");

  filename = string->str;

  g_string_free (string,
                 FALSE);

  return filename;
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


// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_color_combobox_changed (GtkComboBox *widget,
                                                           Object      *owner)
{
  Contest *c = dynamic_cast <Contest *> (owner);

  c->OnColorChanged (widget);
}
