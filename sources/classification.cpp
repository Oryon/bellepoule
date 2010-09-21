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
void Classification::Dump (gchar  *filename,
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

              fprintf (file, "%s", attr->GetUserImage ());
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
