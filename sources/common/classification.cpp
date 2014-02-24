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
#include "player.hpp"

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
void Classification::DumpToHTML (gchar   *filename,
                                 Contest *contest,
                                 GSList  *attr_list)
{
  FILE *file = g_fopen (filename, "w");

  if (file)
  {
    fprintf (file,
             "<html>\n"
             "  <head>\n"
             "    <Title>%s %s - %s - %s - %s</Title>\n"
             "    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>\n"
#include "css.h"
             "  </head>\n\n",
             contest->GetName (),
             contest->GetDate (),
             contest->GetWeapon (),
             contest->GetGender (),
             contest->GetCategory ());

    fprintf (file,
             "  <body>\n"
             "    <div>\n"
             "      <div class=\"Title\">\n"
             "        <center>\n"
             "          <h1>%s</h1>\n"
             "          <h1>%s</h1>\n"
             "          <h1>%s - %s - %s</h1>\n"
             "        </center>\n"
             "      <div>\n"
             "\n"
             "      <div class=\"Round\">\n"
             "        <h3>%s</h3>\n"
             "        <table class=\"Table\">\n",
             contest->GetName (),
             contest->GetDate (),
             contest->GetWeapon (),
             contest->GetGender (),
             contest->GetCategory (),
             gettext ("General classification"));

    // Header
    {
      GSList *current_attr_desc = attr_list;

      fprintf (file, "          <tr class=\"TableHeader\">\n");
      while (current_attr_desc)
      {
        Filter::Layout *layout    = (Filter::Layout *) current_attr_desc->data;
        AttributeDesc  *attr_desc = layout->_desc;

        if (attr_desc->_scope == AttributeDesc::GLOBAL)
        {
          fprintf (file,
                   "            <th>%s</th>\n",
                   attr_desc->_user_name);
        }

        current_attr_desc = g_slist_next (current_attr_desc);
      }
      fprintf (file, "          </tr>\n");
    }

    // Fencers
    {
      GSList *current_player = _player_list;

      for (guint i = 0; current_player; i++)
      {
        Player *player = (Player *) current_player->data;

        if (i%2)
        {
          fprintf (file, "          <tr class=\"OddRow\">\n");
        }
        else
        {
          fprintf (file, "          <tr class=\"EvenRow\">\n");
        }

        {
          GSList *current_attr_desc = attr_list;

          while (current_attr_desc)
          {
            Filter::Layout *layout    = (Filter::Layout *) current_attr_desc->data;
            AttributeDesc  *attr_desc = layout->_desc;

            if (attr_desc->_scope == AttributeDesc::GLOBAL)
            {
              Player::AttributeId  attr_id = Player::AttributeId (attr_desc->_code_name);
              Attribute           *attr    = player->GetAttribute ( &attr_id);

              if (attr)
              {
                gchar *image = attr->GetUserImage (AttributeDesc::LONG_TEXT);

                fprintf (file, "            <td>%s</td>\n", image);
                g_free (image);
              }
            }

            current_attr_desc = g_slist_next (current_attr_desc);
          }
        }
        fprintf (file, "          </tr>\n");
        current_player = g_slist_next (current_player);
      }
    }

    fprintf (file,
             "        </table>\n"
             "      </div>\n"
             "      <div class=\"Footer\">\n"
             "        <h6>Managed by BellePoule <a href=\"http://betton.escrime.free.fr/index.php/bellepoule\">http://betton.escrime.free.fr/index.php/bellepoule</a><h6/>\n"
             "      <div>\n"
             "    </div>\n"
             "  </body>\n");

    fprintf (file, "</html>\n");
    fclose (file);
  }
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

    fprintf (file, "FFF;WIN;competition;%s;individuel\n", contest->GetOrganizer ());
    fprintf (file, "%s;%s;%s;%s;%s;%s\n",
             contest->GetDate (),
             contest->GetWeapon (),
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
