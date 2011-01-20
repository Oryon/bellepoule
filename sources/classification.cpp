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

#include "attribute.hpp"
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
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (_store),
                                        1,
                                        GTK_SORT_ASCENDING);
}

// --------------------------------------------------------------------------------
void Classification::SetSortFunction (GtkTreeIterCompareFunc sort_func,
                                      gpointer               user_data)
{
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (_store),
                                   1,
                                   sort_func,
                                   user_data,
                                   NULL);
  SortDisplay ();
}

// --------------------------------------------------------------------------------
void Classification::DumpToCSV (gchar  *filename,
                                GSList *attr_list)
{
  FILE *file = g_fopen (filename, "w");

  if (file)
  {
    GSList *current_player = _player_list;

    {
      GSList *current_attr_desc = attr_list;

      while (current_attr_desc)
      {
        AttributeDesc *attr_desc = (AttributeDesc *) current_attr_desc->data;

        if (attr_desc->_scope == AttributeDesc::GLOBAL)
        {
          fprintf (file, "%s,", attr_desc->_user_name);
        }

        current_attr_desc = g_slist_next (current_attr_desc);
      }
    }
    fprintf (file, "\n");

    while (current_player)
    {
      Player *player = (Player *) current_player->data;

      {
        GSList *current_attr_desc = attr_list;

        while (current_attr_desc)
        {
          AttributeDesc *attr_desc = (AttributeDesc *) current_attr_desc->data;

          if (attr_desc->_scope == AttributeDesc::GLOBAL)
          {
            Player::AttributeId  attr_id   = Player::AttributeId (attr_desc->_code_name);
            Attribute           *attr      = player->GetAttribute (&attr_id);

            if (attr)
            {
              gchar *image = attr->GetUserImage ();

              fprintf (file, "%s", image);
              g_free (image);
            }
            fprintf (file, ",");
          }

          current_attr_desc = g_slist_next (current_attr_desc);
        }
      }
      fprintf (file, "\n");
      current_player = g_slist_next (current_player);
    }
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
             contest->GetGender (),
             contest->GetCategory (),
             contest->GetName (),
             contest->GetName ());

    while (current_player)
    {
      Player    *player;
      Attribute *status_attr;

      player      = (Player *) current_player->data;
      status_attr = player->GetAttribute (&attr_id);

      if ((status_attr == NULL) || (* ((gchar *) status_attr->GetValue ()) != 'E'))
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
                          "rating");
          fprintf (file, ",");

          fprintf (file, ",;");
        }

        // Results
        {
          WriteFFFString (file,
                          player,
                          "final_rank");
          fprintf (file, ",");

          if (status_attr && (* ((gchar *) status_attr->GetValue ()) != 'Q'))
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
    gsize   bytes_written;
    GError *error = NULL;
    gchar  *result;

    result = g_convert (attr->GetXmlImage (),
                        -1,
                        "ISO-8859-1",
                        "UTF-8",
                        NULL,
                        &bytes_written,
                        &error);

    if (error)
    {
      g_print ("<<GetFFFString>> %s\n", error->message);
      g_clear_error (&error);
    }
    else
    {
      fprintf (file, "%s", result);
      g_free (result);
    }
  }
}
