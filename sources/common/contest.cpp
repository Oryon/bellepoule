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
#include "referees_list.hpp"
#include "checkin_supervisor.hpp"
#include "upload.hpp"

#include "contest.hpp"

typedef enum
{
  FTP_NAME_str,
  FTP_PIXBUF_pix,
  FTP_URL_str,
  FTP_USER_str,
  FTP_PASSWD_str
} FTPColumn;

const gchar *Contest::weapon_image[_nb_weapon] =
{
  N_ ("Sabre"),
  N_ ("Epée"),
  N_ ("Foil"),
  N_ ("Kendo")
};

const gchar *Contest::weapon_xml_image[_nb_weapon] =
{
  "S",
  "E",
  "F",
  "K"
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
Contest::Contest ()
  : Module ("contest.glade")
{
  Object::Dump ();

  _save_timeout_id = 0;

  {
    time_t current_time;

    time (&current_time);
    _id = g_strdup_printf ("%lx", current_time);
  }

  _read_only  = FALSE;
  _notebook   = NULL;
  _level      = NULL;
  _name       = NULL;
  _filename   = NULL;
  _organizer  = NULL;
  _web_site   = NULL;
  _location   = NULL;
  _tournament = NULL;
  _weapon     = 0;
  _category   = 0;
  _gender     = 0;
  _derived    = FALSE;
  _downloader = NULL;

  _state                 = OPERATIONAL;
  _ref_translation_table = NULL;

  _checkin_time = new Time ("checkin");
  _scratch_time = new Time ("scratch");
  _start_time   = new Time ("start");

  _gdk_color = NULL;
  _color     = new Data ("Couleur",
                         (guint) 0);

  _properties_dialog = _glade->GetWidget ("properties_dialog");

  {
    _referees_list = new RefereesList (this);
    Plug (_referees_list,
          _glade->GetWidget ("referees_viewport"),
          NULL);

    gtk_paned_set_position (GTK_PANED (_glade->GetWidget ("hpaned")),
                            0);
    _referee_pane_position = -1;
  }

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

  // FTP repository
  {
    GtkListStore *model  = GTK_LIST_STORE (_glade->GetGObject ("FavoriteFTP"));
    gchar        *path   = g_build_filename (_program_path, "resources", "glade", "escrime_info.jpg", NULL);
    GdkPixbuf    *pixbuf = gdk_pixbuf_new_from_file (path, NULL);
    GtkTreeIter   iter;

    g_free (path);
    gtk_list_store_append (model, &iter);
    gtk_list_store_set (model, &iter,
                        FTP_NAME_str,   "<b><big>Escrime Info  </big></b>",
                        FTP_PIXBUF_pix, pixbuf,
                        FTP_URL_str,    "ftp://www.escrime-info.com/web",
                        FTP_USER_str,   "belle_poule",
                        FTP_PASSWD_str, "tH3MF8huHX",
                        -1);
    g_object_unref (pixbuf);
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
        gtk_widget_set_sensitive (_glade->GetWidget ("save_toolbutton"),
                                  FALSE);
      }

      if (_save_timeout_id > 0)
      {
        g_source_remove (_save_timeout_id);
      }
    }
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
      CheckinSupervisor *checkin;

      _schedule->CreateDefault ();
      checkin = dynamic_cast <CheckinSupervisor *> (_schedule->GetStage (0));

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
void Contest::LoadRemote (const gchar *address)
{
  _downloader = new Downloader (address,
                                OnCompetitionReceived,
                                this);

  _downloader->Start (address,
                      60*2*1000);

  _read_only = TRUE;
}

// --------------------------------------------------------------------------------
gboolean Contest::OnCompetitionReceived (Downloader::CallbackData *cbk_data)
{
  Contest *contest = (Contest *) cbk_data->_user_data;

  contest->OpenMemoryContest (cbk_data);

  return FALSE;
}

// --------------------------------------------------------------------------------
void Contest::OpenMemoryContest (Downloader::CallbackData *cbk_data)
{
  if (_downloader)
  {
    gchar *memory = cbk_data->_downloader->GetData ();

    SetCursor (GDK_WATCH);
    _schedule->RemoveAllStages ();
    if (memory)
    {
      xmlDoc *doc = xmlReadMemory (memory,
                                   strlen (memory),
                                   "noname.xml",
                                   NULL,
                                   0);

      if (doc)
      {
        LoadXmlDoc (doc);
        DisplayProperties ();

        _schedule->Freeze ();

        gtk_widget_hide (_glade->GetWidget ("save_toolbutton"));
        gtk_widget_hide (_glade->GetWidget ("score_frame"));

        gtk_widget_set_sensitive (_glade->GetWidget ("title_entry"),     FALSE);
        gtk_widget_set_sensitive (_glade->GetWidget ("organizer_entry"), FALSE);
        gtk_widget_set_sensitive (_glade->GetWidget ("web_site_entry"),  FALSE);
        gtk_widget_set_sensitive (_glade->GetWidget ("location_entry"),  FALSE);
        gtk_widget_set_sensitive (_glade->GetWidget ("calendar_button"), FALSE);
        gtk_widget_set_sensitive (_weapon_combo,   FALSE);
        gtk_widget_set_sensitive (_gender_combo,   FALSE);
        gtk_widget_set_sensitive (_category_combo, FALSE);

        _checkin_time->HideProperties (_glade);
        _scratch_time->HideProperties (_glade);
        _start_time->HideProperties   (_glade);

        gtk_notebook_remove_page (GTK_NOTEBOOK (_glade->GetWidget ("properties_notebook")),
                                  1);
      }
    }

    ResetCursor ();

    while (gtk_events_pending ())
    {
      gtk_main_iteration ();
    }
  }
}

// --------------------------------------------------------------------------------
void Contest::LoadXmlDoc (xmlDoc *doc)
{
  _state = LOADING;

  _ref_translation_table = g_hash_table_new (NULL,
                                             NULL);

  {
    gboolean  score_stuffing_policy = FALSE;
    gboolean  need_post_processing  = FALSE;

    xmlXPathInit ();

    {
      xmlXPathContext *xml_context = xmlXPathNewContext (doc);
      xmlXPathObject  *xml_object;
      xmlNodeSet      *xml_nodeset;

      xml_object = xmlXPathEval (BAD_CAST "/CompetitionIndividuelle", xml_context);

      if (xml_object->nodesetval->nodeNr == 0)
      {
        xmlXPathFreeObject (xml_object);
        xml_object = xmlXPathEval (BAD_CAST "/BaseCompetitionIndividuelle", xml_context);

        need_post_processing = TRUE;
      }

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
          for (guint i = 0; i < _nb_weapon; i++)
          {
            if (strcmp (attr, weapon_xml_image[i]) == 0)
            {
              _weapon = i;
            }
          }
          xmlFree (attr);
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
          xmlFree (attr);
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

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "score_stuffing");
        if (attr)
        {
          score_stuffing_policy = (gboolean) atoi (attr);
          xmlFree (attr);
        }
      }
      xmlXPathFreeObject  (xml_object);
      xmlXPathFreeContext (xml_context);
    }

    _schedule->Load (doc,
                     _referees_list);

    if (score_stuffing_policy)
    {
      _schedule->SetScoreStuffingPolicy (score_stuffing_policy);
    }

    xmlFreeDoc (doc);

    if (need_post_processing)
    {
      CheckinSupervisor *checkin = dynamic_cast <CheckinSupervisor *> (_schedule->GetStage (0));

      checkin->ConvertFromBaseToResult ();
    }
  }

  {
    g_hash_table_destroy (_ref_translation_table);
    _ref_translation_table = NULL;
  }

  _state = OPERATIONAL;
  _schedule->OnLoadingCompleted ();
}

// --------------------------------------------------------------------------------
Contest::~Contest ()
{
  _state = LEAVING;

  g_free (_level);
  g_free (_id);
  g_free (_name);
  g_free (_filename);
  g_free (_organizer);
  g_free (_web_site);
  g_free (_location);

  Object::TryToRelease (_checkin_time);
  Object::TryToRelease (_scratch_time);
  Object::TryToRelease (_start_time);

  Object::TryToRelease (_color);

  Object::TryToRelease (_referees_list);

  Object::TryToRelease (_schedule);

  Object::Dump ();

  if (_save_timeout_id > 0)
  {
    g_source_remove (_save_timeout_id);
  }

  if (_tournament)
  {
    _tournament->OnContestDeleted (this);
  }

  if (_downloader)
  {
    _downloader->Kill    ();
    _downloader->Release ();
  }
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
    CheckinSupervisor *checkin = dynamic_cast <CheckinSupervisor *> (_schedule->GetStage (0));

    if (checkin)
    {
      Player::AttributeId  start_rank_attr    ("start_rank");
      Player::AttributeId  previous_rank_attr ("previous_stage_rank", checkin);

      checkin->Add (fencer);
      fencer->SetAttributeValue (&start_rank_attr,
                                 rank);
      fencer->SetAttributeValue (&previous_rank_attr,
                                 rank);
      checkin->UseInitialRank ();
    }
  }
}

// --------------------------------------------------------------------------------
void Contest::AddReferee (Player *referee)
{
  _referees_list->Add           (referee);
  _referees_list->OnListChanged ();
}

// --------------------------------------------------------------------------------
void Contest::ImportReferees (GSList *imported_list)
{
  GSList *attr_list = NULL;
  Player::AttributeId  name_attr_id       ("name");
  Player::AttributeId  first_name_attr_id ("first_name");

  attr_list = g_slist_prepend (attr_list, &first_name_attr_id);
  attr_list = g_slist_prepend (attr_list, &name_attr_id);

  while (imported_list)
  {
    Player *imported = (Player *) imported_list->data;

    if (imported->GetWeaponCode () == GetWeaponCode ())
    {
      GSList *current  = _referees_list->GetList ();

      while (current)
      {
        Player *referee = (Player *) current->data;

        if (Player::MultiCompare (imported,
                                  referee,
                                  attr_list) == 0)
        {
          break;
        }
        current = g_slist_next (current);
      }

      if (current == NULL)
      {
        AddReferee (imported);
      }
    }

    imported_list = g_slist_next (imported_list);
  }

  g_slist_free (attr_list);
}

// --------------------------------------------------------------------------------
Contest *Contest::Create ()
{
  Contest *contest = new Contest ();

  contest->ChooseColor ();

  contest->_schedule->SetScoreStuffingPolicy (FALSE);

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
  contest->_location = g_key_file_get_string (_config_file,
                                              "Competiton",
                                              "default_location",
                                              NULL);

  contest->FillInProperties ();

  {
    if (gtk_dialog_run (GTK_DIALOG (contest->_properties_dialog)) == TRUE)
    {
      contest->_schedule->CreateDefault ();
      contest->ReadProperties ();
      gtk_widget_hide (contest->_properties_dialog);
    }
    else
    {
      gtk_widget_hide (contest->_properties_dialog);
      Object::TryToRelease (contest);
      contest = NULL;
    }
  }

  return contest;
}

// --------------------------------------------------------------------------------
Contest *Contest::Duplicate ()
{
  Contest *contest = new Contest ();

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

  _checkin_time->Copy (contest->_checkin_time);
  _scratch_time->Copy (contest->_scratch_time);
  _start_time->Copy   (contest->_start_time);

  return contest;
}

// --------------------------------------------------------------------------------
void Contest::LatchPlayerList ()
{
  CheckinSupervisor *checkin = dynamic_cast <CheckinSupervisor *> (_schedule->GetStage (0));

  if (checkin)
  {
    Player::AttributeId  rank_attr ("previous_stage_rank", checkin);

    checkin->OnListChanged ();
  }
}

// --------------------------------------------------------------------------------
Player *Contest::GetRefereeFromRef (guint ref)
{
  if (_ref_translation_table)
  {
    ref = GPOINTER_TO_UINT (g_hash_table_lookup (_ref_translation_table,
                                                 (gconstpointer) ref));

  }

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

  color_to_use = g_key_file_get_integer (_config_file,
                                         "Competiton",
                                         "color_to_use",
                                         NULL);
  if (color_to_use >= (gint) (g_list_length (_color_list)))
  {
    color_to_use = 0;
  }

  _color->_value = color_to_use;

  color_to_use++;
  if (color_to_use >= (gint) (g_list_length (_color_list)))
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
  _tournament = dynamic_cast <Tournament *> (_owner);
}

// --------------------------------------------------------------------------------
Player *Contest::Share (Player *referee)
{
  Player *original;
  guint   old_ref = referee->GetRef ();

  referee->SetWeaponCode (GetWeaponCode ());

  original = _tournament->Share (referee,
                                 this);
  if (_ref_translation_table)
  {
    g_hash_table_insert (_ref_translation_table,
                         (gpointer) old_ref,
                         (gpointer) referee->GetRef ());
  }

  return original;
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

  entry = GTK_ENTRY (_glade->GetWidget ("location_entry"));
  str = (gchar *) gtk_entry_get_text (entry);
  g_free (_location);
  _location = g_strdup (str);

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

    g_key_file_set_string (_config_file,
                           "Competiton",
                           "default_location",
                           _location);
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
  _notebook = GTK_NOTEBOOK (to);

  {
    GtkWidget *title = _glade->GetWidget ("notebook_title");

    gtk_notebook_append_page (_notebook,
                              GetRootWidget (),
                              title);
    g_object_unref (title);
  }

  if (_derived == FALSE)
  {
    gtk_notebook_set_current_page (_notebook,
                                   -1);
  }

  gtk_notebook_set_tab_reorderable (_notebook,
                                    GetRootWidget (),
                                    TRUE);

  DisplayProperties ();
}

// --------------------------------------------------------------------------------
void Contest::Publish ()
{
  if (_schedule->ScoreStuffingIsAllowed () == FALSE)
  {
    Upload *upload = new Upload (_filename,
                                 gtk_entry_get_text (GTK_ENTRY (_glade->GetWidget ("url_entry"))),
                                 gtk_entry_get_text (GTK_ENTRY (_glade->GetWidget ("user_entry"))),
                                 gtk_entry_get_text (GTK_ENTRY (_glade->GetWidget ("passwd_entry"))));

    upload->Start ();
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
  if ((_filename == NULL) && (_read_only == FALSE))
  {
    _filename = GetSaveFileName (gettext ("Choose a file..."),
                                 "default_dir_name");
  }

  if (_filename)
  {
    Save (_filename);

    {
      gchar *uri = g_filename_to_uri (_filename,
                                      NULL,
                                      NULL);

      gtk_recent_manager_add_item (gtk_recent_manager_get_default (),
                                   uri);
      g_free (uri);
    }

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
                             BAD_CAST "CompetitionIndividuelle",
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
                                 BAD_CAST "CompetitionIndividuelle");

      {
        _color->Save (xml_writer);

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
                                           BAD_CAST "score_stuffing",
                                           "%d", _schedule->ScoreStuffingIsAllowed ());
        xmlTextWriterWriteAttribute (xml_writer,
                                     BAD_CAST "Lieu",
                                     BAD_CAST _location);
      }

      _schedule->Save (xml_writer,
                       _referees_list);

      xmlTextWriterEndElement (xml_writer);
      xmlTextWriterEndDocument (xml_writer);

      xmlFreeTextWriter (xml_writer);
    }

    gtk_widget_set_sensitive (_glade->GetWidget ("save_toolbutton"),
                              FALSE);
  }
}

// --------------------------------------------------------------------------------
gchar *Contest::GetSaveFileName (gchar       *title,
                                 const gchar *config_key)
{
  GtkWidget *chooser;
  char      *filename = NULL;

  chooser = GTK_WIDGET (gtk_file_chooser_dialog_new (gettext ("Choose a file..."),
                                                     NULL,
                                                     GTK_FILE_CHOOSER_ACTION_SAVE,
                                                     GTK_STOCK_CANCEL,
                                                     GTK_RESPONSE_CANCEL,
                                                     GTK_STOCK_SAVE_AS,
                                                     GTK_RESPONSE_ACCEPT,
                                                     NULL));
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

  {
    char *name = GetDefaultFileName ();

    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (chooser),
                                       name);
    g_free (name);
  }

  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
  {
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));

    if (filename)
    {
      gchar *dirname = g_path_get_dirname (filename);

      g_key_file_set_string (_config_file,
                             "Competiton",
                             config_key,
                             dirname);
      g_free (dirname);
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
                         "font", "Sans Bold 2.5px", NULL);
  }

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

  if (_organizer)
  {
    goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                         _organizer,
                         98.0, 2.0,
                         -1.0,
                         GTK_ANCHOR_EAST,
                         "fill-color", "black",
                         "font", "Sans Bold 2.5px", NULL);
  }

  {
    goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                         GetDate (),
                         98.0, 5.0,
                         -1.0,
                         GTK_ANCHOR_EAST,
                         "fill-color", "black",
                         "font", "Sans Bold 2.5px", NULL);
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
void Contest::on_referees_toolbutton_toggled (GtkToggleToolButton *w)
{
  GtkPaned *paned = GTK_PANED (_glade->GetWidget ("hpaned"));

  if (gtk_toggle_tool_button_get_active (w))
  {
    gtk_paned_set_position (paned,
                            _referee_pane_position);
  }
  else
  {
    _referee_pane_position = gtk_paned_get_position (paned);
    gtk_paned_set_position (paned,
                            0);
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
  if (gtk_dialog_run (GTK_DIALOG (_properties_dialog)) == TRUE)
  {
    ReadProperties ();
    MakeDirty ();
  }
  gtk_widget_hide (_properties_dialog);
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
  Save ();
  Release ();
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
gchar Contest::GetWeaponCode ()
{
  return *weapon_xml_image[_weapon];
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
                          gettext (weapon_image[_weapon]),
                          gettext (gender_image[_gender]),
                          gettext (category_image[_category]));
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

// --------------------------------------------------------------------------------
void Contest::on_ftp_changed (GtkComboBox *widget)
{
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (widget,
                                     &iter))
  {
    gchar *url;
    gchar *user;
    gchar *passwd;

    gtk_tree_model_get (gtk_combo_box_get_model (widget),
                        &iter,
                        FTP_URL_str,    &url,
                        FTP_USER_str,   &user,
                        FTP_PASSWD_str, &passwd,
                        -1);

    gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("url_entry")),
                        url);
    gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("user_entry")),
                        user);
    gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("passwd_entry")),
                        passwd);

    g_free (url);
    g_free (user);
    g_free (passwd);
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_ftp_comboboxentry_changed (GtkComboBox *widget,
                                                              Object      *owner)
{
  Contest *c = dynamic_cast <Contest *> (owner);

  c->on_ftp_changed (widget);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_referees_toolbutton_toggled (GtkToggleToolButton *widget,
                                                                Object              *owner)
{
  Contest *c = dynamic_cast <Contest *> (owner);

  c->on_referees_toolbutton_toggled (widget);
}
