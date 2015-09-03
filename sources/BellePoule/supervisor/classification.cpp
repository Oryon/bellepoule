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
#include <glib/gstdio.h>

#include "util/attribute.hpp"

#include "contest.hpp"
#include "util/player.hpp"
#include "application/weapon.hpp"

#include "classification.hpp"

// --------------------------------------------------------------------------------
Classification::Classification (Filter *filter)
  : PlayersList ("classification.glade",
                 NO_RIGHT)
{
  if (filter)
  {
    filter->SetOwner (this);
  }
  SetFilter (filter);
}

// --------------------------------------------------------------------------------
Classification::~Classification ()
{
}

// --------------------------------------------------------------------------------
void Classification::OnPlugged ()
{
  OnAttrListUpdated ();
}

// --------------------------------------------------------------------------------
void Classification::SortDisplay ()
{
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (gtk_tree_view_get_model (_tree_view)),
                                        1,
                                        GTK_SORT_ASCENDING);
}

// --------------------------------------------------------------------------------
void Classification::SetSortFunction (GtkTreeIterCompareFunc sort_func,
                                      gpointer               user_data)
{
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (gtk_tree_view_get_model (_tree_view)),
                                   1,
                                   sort_func,
                                   user_data,
                                   NULL);
}

// --------------------------------------------------------------------------------
void Classification::DumpToFFF (gchar   *filename,
                                Contest *contest)
{
  FILE *file = g_fopen (filename, "w");

  if (file)
  {
    Player::AttributeId  attr_id ("global_status");
    GSList              *current_player = _player_list;
    Weapon              *weapon = contest->GetWeapon ();

    fprintf (file, "FFF;WIN;competition;%s;individuel\n", contest->GetOrganizer ());
    fprintf (file, "%s;%s;%s;%s;%s;%s\n",
             contest->GetDate (),
             weapon->GetImage (),
             contest->GetGenderCode (),
             contest->GetCategory (),
             contest->GetName (),
             contest->GetName ());

    while (current_player)
    {
      Player    *player;
      Attribute *status_attr;

      player      = (Player *) current_player->data;
      status_attr = player->GetAttribute (&attr_id);

      if ((status_attr == NULL) || (* (status_attr->GetStrValue ()) != 'E'))
      {
        // General
        {
          WriteFFFString (file,
                          player,
                          "name");
          fprintf (file, ",");

          WriteFFFString (file,
                          player,
                          "first_name");
          fprintf (file, ",");

          WriteFFFString (file,
                          player,
                          "birth_date");
          fprintf (file, ",");

          WriteFFFString (file,
                          player,
                          "gender");
          fprintf (file, ",");

          WriteFFFString (file,
                          player,
                          "country");
          fprintf (file, ",");

          fprintf (file, ";");
        }

        // FIE
        {
          fprintf (file, ",,;");
        }

        // National federation
        {
          WriteFFFString (file,
                          player,
                          "licence");
          fprintf (file, ",");

          WriteFFFString (file,
                          player,
                          "league");
          fprintf (file, ",");

          WriteFFFString (file,
                          player,
                          "club");
          fprintf (file, ",");

          WriteFFFString (file,
                          player,
                          "ranking");
          fprintf (file, ",");

          fprintf (file, ",;");
        }

        // Results
        {
          WriteFFFString (file,
                          player,
                          "final_rank");
          fprintf (file, ",");

          if (status_attr && (* (status_attr->GetStrValue ()) != 'Q'))
          {
            fprintf (file, "p");
          }
          else
          {
            fprintf (file, "t");
          }
        }

        fprintf (file, "\n");
      }
      current_player = g_slist_next (current_player);
    }
    fclose (file);
  }
}

// --------------------------------------------------------------------------------
void Classification::WriteFFFString (FILE        *file,
                                     Player      *player,
                                     const gchar *attr_name)
{
  Player::AttributeId  attr_id (attr_name);
  Attribute           *attr = player->GetAttribute (&attr_id);

  if (attr)
  {
    GError *error = NULL;
    gchar  *result;

    {
      gsize  bytes_written;
      gchar *xml_image = attr->GetXmlImage ();

      if (strcmp (attr_name, "birth_date") == 0)
      {
        GDate date;

        g_date_set_parse (&date,
                          attr->GetStrValue ());

        if (g_date_valid (&date))
        {
          gchar buffer[50];

          g_date_strftime (buffer,
                           sizeof (buffer),
                           "%d/%m/%Y",
                           &date);
          g_free (xml_image);
          xml_image = g_strdup (buffer);
        }
      }

      result = g_convert (xml_image,
                          -1,
                          "ISO-8859-1",
                          "UTF-8",
                          NULL,
                          &bytes_written,
                          &error);
      g_free (xml_image);
    }

    if (error)
    {
      g_print ("<<GetFFFString>> %s\n", error->message);
      g_error_free (error);
    }
    else if (result)
    {
      gchar *current = result;

      while (*current)
      {
        // Workaround to avoid CLAS issue
        if (*current == '\240')
        {
          *current = ' ';
        }
        current++;
      }

      fprintf (file, "%s", result);
      g_free (result);
    }
  }
}

// --------------------------------------------------------------------------------
gboolean Classification::IsTableBorder (guint place)
{
  if (place > 1)
  {
    guint bit1_count = 0;

    while (place)
    {
      bit1_count += (place & 1L);
      place >>= 1;
    }

    return (bit1_count == 1);
  }

  return FALSE;
}
