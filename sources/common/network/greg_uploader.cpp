// Copyright (C) 2011 Yannick Le Roux.
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

#include "greg_uploader.hpp"

const gchar *gender_greg[] =
{
  "H",
  "D",
  "H"
};

static const guint JOB_COUNT = 2;

namespace Net
{
  // --------------------------------------------------------------------------------
  GregUploader::GregUploader (const gchar *url,
                              const gchar *www)
    : FileUploader (url, nullptr, nullptr, www)
  {
    _current_job  = 0;
    _local_file   = nullptr;
    _remote_file  = nullptr;
    _job_url      = g_new0 (gchar *, JOB_COUNT);
    _has_location = FALSE;

    g_datalist_init (&_form_fields);
  }

  // --------------------------------------------------------------------------------
  GregUploader::~GregUploader ()
  {
    g_datalist_clear (&_form_fields);

    for (guint i = 0; i < JOB_COUNT; i++)
    {
      g_free (_job_url[i]);
    }
    g_free (_job_url);

    g_free (_local_file);
    g_free (_remote_file);
  }

  // --------------------------------------------------------------------------------
  void GregUploader::UploadFile (const gchar *file_path)
  {
    if (_has_location)
    {
      if (_remote_file == nullptr)
      {
        _remote_file = g_path_get_basename (file_path);
      }

      {
        gchar *escaped  = g_uri_escape_string (_remote_file,
                                               nullptr,
                                               FALSE);

        _job_url[0] = g_strdup_printf ("http://www.escrime-info.com/gregxml/Greg/envoyer.php?fichier=%s",
                                       escaped);
        _job_url[1] = g_strdup_printf ("http://www.escrime-info.com/gregxml/Greg/envoyer.php?fichier=./temp/%s&valide=1",
                                       escaped);

        g_free (escaped);
      }

      FileUploader::UploadFile (file_path);
    }
  }

  // --------------------------------------------------------------------------------
  const gchar *GregUploader::GetUrl ()
  {
    if (_current_job < JOB_COUNT)
    {
      return _job_url[_current_job];
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  void GregUploader::SetCurlOptions (CURL *curl)
  {
    FileUploader::SetCurlOptions (curl);

    _form_head = nullptr;
    _form_tail = nullptr;

    if (_current_job == 0)
    {
#ifdef G_OS_WIN32
      _local_file = g_win32_locale_filename_from_utf8 (_file_path);
#else
      _local_file = g_strdup (_file_path);
#endif
      curl_formadd (&_form_head,
                    &_form_tail,
                    CURLFORM_PTRNAME,  "ficXML",
                    CURLFORM_FILE,     _local_file,
                    CURLFORM_FILENAME, _remote_file,
                    CURLFORM_END);
    }
    else if (_current_job == 1)
    {
      g_datalist_foreach (&_form_fields,
                          (GDataForeachFunc) FeedFormField,
                          this);
    }

    curl_formadd (&_form_head,
                  &_form_tail,
                  CURLFORM_PTRNAME,      "submit",
                  CURLFORM_COPYCONTENTS, "send",
                  CURLFORM_END);

    curl_easy_setopt (curl, CURLOPT_HTTPPOST,  _form_head);
    curl_easy_setopt (curl, CURLOPT_USERAGENT, "BellePoule/5.0");
  }

  // --------------------------------------------------------------------------------
  void GregUploader::Looper ()
  {
    for (_current_job = 0; _current_job < JOB_COUNT; _current_job++)
    {
      Upload ();
      curl_formfree (_form_head);
    }
  }

  // --------------------------------------------------------------------------------
  void GregUploader::FeedFormField (GQuark        quark,
                                    const gchar  *value,
                                    GregUploader *uploader)
  {
    curl_formadd (&uploader->_form_head,
                  &uploader->_form_tail,
                  CURLFORM_COPYNAME,     g_quark_to_string (quark),
                  CURLFORM_COPYCONTENTS, value,
                  CURLFORM_END);
  }

  // --------------------------------------------------------------------------------
  void GregUploader::SetTeamEvent (gboolean is_team_event)
  {
    if (is_team_event)
    {
      g_datalist_set_data (&_form_fields,
                           "type_competition",
                           (gpointer) "CompetitionParEquipes");
      g_datalist_set_data (&_form_fields,
                           "indivouequipe",
                           (gpointer) "equipe");
    }
    else
    {
      g_datalist_set_data (&_form_fields,
                           "type_competition",
                           (gpointer) "CompetitionIndividuelle");
      g_datalist_set_data (&_form_fields,
                           "indivouequipe",
                           (gpointer) "indiv");
    }
  }

  // --------------------------------------------------------------------------------
  void GregUploader::SetRunningState (gboolean is_over)
  {
    if (is_over)
    {
      g_datalist_set_data (&_form_fields,
                           "status",
                           (gpointer) "D");
    }
    else
    {
      g_datalist_set_data (&_form_fields,
                           "status",
                           (gpointer) "L");
    }
  }

  // --------------------------------------------------------------------------------
  void GregUploader::SetDate (guint day,
                              guint month,
                              guint year)
  {
    g_datalist_set_data_full (&_form_fields,
                              "dp-normal-1",
                              g_strdup_printf ("%02d.%02d.%d", day, month, year),
                              g_free);
  }

  // --------------------------------------------------------------------------------
  void GregUploader::SetLocation (const gchar *location,
                                  const gchar *label)
  {
    gchar *lieu = nullptr;

    if (location && (location[0] != '\0'))
    {
      _has_location = TRUE;

      {
        GString *string = g_string_new (nullptr);

        string = g_string_append (string, location);

        if (label && (label[0] != '\0'))
        {
          string = g_string_append_c (string, '[');
          string = g_string_append (string, label);
          string = g_string_append_c (string, ']');
        }

        lieu = string->str;

        g_string_free (string,
                       FALSE);
      }
    }

    g_datalist_set_data_full (&_form_fields,
                              "lieu",
                              lieu,
                              g_free);
  }

  // --------------------------------------------------------------------------------
  void GregUploader::SetLevel (const gchar *level)
  {
    g_datalist_set_data_full (&_form_fields,
                              "niveau",
                              g_strdup (level),
                              g_free);
  }

  // --------------------------------------------------------------------------------
  void GregUploader::SetDivision (const gchar *division)
  {
    g_datalist_set_data_full (&_form_fields,
                              "Division",
                              g_strdup (division),
                              g_free);
  }

  // --------------------------------------------------------------------------------
  void GregUploader::SetWeapon (const gchar *weapon)
  {
    g_datalist_set_data_full (&_form_fields,
                              "arme",
                              g_strdup (weapon),
                              g_free);
  }

  // --------------------------------------------------------------------------------
  void GregUploader::SetGender (guint gender)
  {
    g_datalist_set_data (&_form_fields,
                         "genre",
                         (gpointer) gender_greg[gender]);
  }

  // --------------------------------------------------------------------------------
  void GregUploader::SetCategory (const gchar *category)
  {
    g_datalist_set_data_full (&_form_fields,
                              "categorie",
                              g_strdup (category),
                              g_free);
  }
}
