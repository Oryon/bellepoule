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

#include <errno.h>
#include <fcntl.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <zip.h>

#include "zipper.hpp"

// --------------------------------------------------------------------------------
Zipper::Zipper (const gchar *zip_name)
  : Object ("Zipper")
{
  int    zip_error;
  gchar *altered_name;

   _zip_name      = zip_name;
   _temp_zip_name = g_strdup_printf ("%s__", zip_name);
   altered_name   = g_strdup (_temp_zip_name);

  MakeLocaleFilenameFromUtf8 (&altered_name);

  _to_archive = zip_open (altered_name,
                          ZIP_EXCL | ZIP_CREATE,
                          &zip_error);

  if (_to_archive == nullptr)
  {
    zip_error_t error;

    zip_error_init_with_code (&error, zip_error);
    g_warning ("zip_fdopen error: %s\n", zip_error_strerror (&error));
    zip_error_fini (&error);
  }

  g_free (altered_name);
}

// --------------------------------------------------------------------------------
Zipper::~Zipper ()
{
  if (_to_archive)
  {
    if (zip_close (_to_archive) < 0)
    {
      g_warning ("zip_close (%s): %s\n", _temp_zip_name, zip_strerror (_to_archive));
    }
    else
    {
      g_rename (_zip_name,
                "__old.zip");
      g_rename (_temp_zip_name,
                _zip_name);
      g_remove ("__old.zip");
    }
  }

  g_free (_temp_zip_name);
}

// --------------------------------------------------------------------------------
void Zipper::AddEntry (zip_t       *archive,
                       zip_source  *source,
                       const gchar *title)
{
  gchar *base_name;

  {
    guint      count = zip_get_num_entries (archive, 0);
    GDateTime *now   = g_date_time_new_now_local ();
    gchar     *iso   = g_date_time_format (now, "[%d_%m_%Y]-[%H_%M_%S]");

    base_name = g_strdup_printf ("%02d-%s-%s.cotcot", count, iso, title);

    g_date_time_unref (now);
    g_free (iso);
  }

  if (zip_file_add (archive,
                    base_name,
                    source,
                    ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8) < 0)
  {
    g_warning ("zip_file_add (%s) error: %s\n", base_name, zip_strerror (archive));
    zip_source_free (source);
  }

  g_free (base_name);
}

// --------------------------------------------------------------------------------
void Zipper::Archive (const gchar *what,
                      const gchar *title)
{
  if (_to_archive)
  {
    gchar *source_name = g_strdup (what);

    MakeLocaleFilenameFromUtf8 (&source_name);

    {
      zip_source_t *source = zip_source_file (_to_archive,
                                              source_name,
                                              0,
                                              0);
      if (source == nullptr)
      {
        g_warning ("zip_source_file error: %s\n", zip_strerror (_to_archive));
      }
      else
      {
        AddEntry (_to_archive,
                  source,
                  title);
      }
    }

    g_free (source_name);
  }
}

// --------------------------------------------------------------------------------
void Zipper::Archive2 (const gchar *what,
                       const gchar *title)
{
  gchar *data;
  gsize  data_length;

  if (g_file_get_contents (_zip_name,
                           &data,
                           &data_length,
                           nullptr))
  {
    zip_source_t *source;
    zip_error_t   error;

    zip_error_init (&error);
    source = zip_source_buffer_create (data,
                                       data_length,
                                       0,
                                       &error);
    if (source == nullptr)
    {
      g_warning ("zip_source_buffer_create: %s\n", zip_error_strerror (&error));
    }
    else
    {
      zip_t *from_archive = zip_open_from_source (source,
                                                  0,
                                                  &error);

      if (from_archive == nullptr)
      {
        g_warning ("zip_open_from_source: %s\n", zip_error_strerror (&error));
        zip_source_free (source);
      }
      else
      {
        zip_int64_t num_entries = zip_get_num_entries (from_archive,
                                                       ZIP_FL_UNCHANGED);

        for (zip_int64_t i = 0; i < num_entries; i++)
        {
          zip_stat_t entry_stat;

          if (zip_stat_index (from_archive,
                              i,
                              0,
                              &entry_stat) == -1)
          {
            g_warning ("zip_stat_index: %s", zip_strerror (from_archive));
          }
          else
          {
            zip_file_t *entry = zip_fopen_index (from_archive,
                                                 i,
                                                 ZIP_FL_UNCHANGED);

            if (entry == nullptr)
            {
              g_warning ("zip_fopen_index: %s", zip_strerror (from_archive));
            }
            else
            {
              gchar *content = g_new (gchar, entry_stat.size);

              if (zip_fread (entry,
                             content,
                             entry_stat.size) != (zip_int64_t) entry_stat.size)
              {
                g_warning ("zip_fread: %s", zip_strerror (from_archive));
              }
              else
              {
                zip_source *dest_source = zip_source_buffer (_to_archive,
                                                             content,
                                                             entry_stat.size,
                                                             1);
                if (dest_source == nullptr)
                {
                  g_warning ("zip_source_buffer: %s", zip_strerror (_to_archive));
                }
                else if (zip_file_add (_to_archive,
                                       entry_stat.name,
                                       dest_source,
                                       ZIP_FL_OVERWRITE) == -1)
                {
                  g_warning ("zip_file_add: %s", zip_strerror (_to_archive));
                }
              }

              zip_fclose (entry);
            }
          }
        }

        if (zip_close (from_archive) < 0)
        {
          g_warning ("zip_close: %s\n", zip_strerror (from_archive));
          zip_source_free (source);
        }
      }
    }

    zip_error_fini (&error);
    g_free (data);
  }

  Archive (what,
           title);
}
