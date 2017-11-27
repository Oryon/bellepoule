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

#include "user_config.hpp"

// --------------------------------------------------------------------------------
UserConfig::UserConfig (const gchar *app_name,
                        gboolean     read_only)
  : Object ("UserConfig")
{
  gchar *dir_path  = g_strdup_printf ("%s/%s", g_get_user_config_dir (), app_name);

  _file_path = g_strdup_printf ("%s/config.ini", dir_path);

  _read_only = read_only;

  _key_file = g_key_file_new ();

  if (g_key_file_load_from_file (_key_file,
                                 _file_path,
                                 G_KEY_FILE_KEEP_COMMENTS,
                                 NULL) == FALSE)
  {
    g_mkdir_with_parents (dir_path,
                          0700);
  }

  g_free (dir_path);
}

// --------------------------------------------------------------------------------
UserConfig::~UserConfig ()
{
  Save ();

  g_free (_file_path);
  g_key_file_free (_key_file);
}

// --------------------------------------------------------------------------------
void UserConfig::Save ()
{
  if (_read_only == FALSE)
  {
    GError *error = NULL;
    gsize   config_length;
    gchar  *config = g_key_file_to_data (_key_file,
                                         &config_length,
                                         &error);

    if (error)
    {
      g_error ("g_key_file_to_data: %s", error->message);
      g_error_free (error);
    }
    else if (g_file_set_contents (_file_path,
                                  config,
                                  config_length,
                                  &error) == FALSE)
    {
      g_error ("g_file_set_contents: %s", error->message);
      g_error_free (error);
    }
    g_free (config);
  }
}

// --------------------------------------------------------------------------------
gboolean UserConfig::IsEmpty ()
{
  gchar    **contents = g_key_file_get_groups (_key_file, NULL);
  gboolean   is_empty = (contents == NULL);

  g_strfreev (contents);

  return is_empty;
}
