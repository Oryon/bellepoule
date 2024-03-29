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

#include <glib/gstdio.h>
#include <glib/gstdio.h>
#include <libxml/xpath.h>

#include "util/attribute.hpp"
#include "util/player.hpp"
#include "util/xml_scheme.hpp"

#include "contest.hpp"
#include "application/weapon.hpp"
#include "network/message.hpp"
#include "network/ring.hpp"

#include "classification.hpp"

// --------------------------------------------------------------------------------
Classification::Classification ()
  : Object ("Classification"),
    PlayersList ("classification.glade", nullptr, NO_RIGHT)
{
  _fff_place_shifting = 0;

  {
    Net::Message *parcel = Object::Disclose ("BellePoule::Classification");

    parcel->Set ("channel", (guint) Net::Ring::Channel::WEB_SOCKET);
  }
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
void Classification::FeedParcel (Net::Message *parcel)
{
  xmlBuffer *xml_buffer = xmlBufferCreate ();

  {
    Player::AttributeId elo_attr_id ("elo");
    Player::AttributeId quest_attr_id("score_quest", GetDataOwner());
    Player::AttributeId vict_attr_id("victories_count", GetDataOwner());
    Player::AttributeId tb_attr_id("tiebreaker_quest", GetDataOwner());
    XmlScheme *xml_scheme = new XmlScheme (xml_buffer);

    xml_scheme->StartElement ("Classement");
    for (GList *p = _player_list; p != nullptr; p = g_list_next (p))
    {
      Player *fencer = (Player *) p->data;

      xml_scheme->StartElement ("Tireur");

      {
        gchar *ref = g_strdup_printf ("%d", fencer->GetRef ());

        xml_scheme->WriteAttribute ("Ref",
                                    ref);
        g_free (ref);
      }

      {
        Attribute *elo_attr = fencer->GetAttribute(&elo_attr_id);
        gchar *elo = g_strdup_printf("%d", elo_attr->GetIntValue());

        xml_scheme->WriteAttribute("Elo",
                                   elo);
        g_free(elo);
      }

      {
        Attribute *quest_attr = fencer->GetAttribute(&quest_attr_id);
        gchar *quest = g_strdup_printf("%u", quest_attr ? quest_attr->GetUIntValue() : 0);

        xml_scheme->WriteAttribute("Quest",
                                   quest);
        g_free(quest);
      }

      {
        Attribute *vict_attr = fencer->GetAttribute(&vict_attr_id);
        gchar *vict = g_strdup_printf("%u", vict_attr ? vict_attr->GetUIntValue() : 0);

        xml_scheme->WriteAttribute("Vict",
                                   vict);
        g_free(vict);
      }

      {
        Attribute *tb_attr = fencer->GetAttribute(&tb_attr_id);
        gchar *tb = g_strdup_printf("%u", tb_attr ? tb_attr->GetUIntValue() : 0);

        xml_scheme->WriteAttribute("TB",
                                   tb);
        g_free(tb);
      }

      xml_scheme->EndElement ();
    }
      xml_scheme->EndElement ();

    xml_scheme->Release ();
  }

  parcel->Set ("xml", (const gchar *) xml_buffer->content);

  xmlBufferFree (xml_buffer);
}

// --------------------------------------------------------------------------------
void Classification::Conceal ()
{
  Object::Conceal ();
}

// --------------------------------------------------------------------------------
void Classification::Spread ()
{
  Object::Spread ();
}

// --------------------------------------------------------------------------------
void Classification::Recall ()
{
  Object::Recall ();
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
                                   nullptr);
}

// --------------------------------------------------------------------------------
void Classification::DumpToFFF (gchar   *filename,
                                Contest *contest,
                                guint    place_shifting)
{
  FILE *file = g_fopen (filename, "w");

  if (file)
  {
    Player::AttributeId  attr_id ("global_status");
    GList               *current_player = _player_list;
    Weapon              *weapon = contest->GetWeapon ();

    fprintf (file, "FFF;WIN;competition;%s;individuel\n", contest->GetOrganizer ());
    {
      gsize  bytes_written;
      gchar *iso8858;
      gchar *utf8 = g_strdup_printf ("%s;%s;%s;%s;%s;%s\n",
                                     contest->GetDate (),
                                     weapon->GetImage (),
                                     contest->GetGenderCode (),
                                     contest->GetCategory (),
                                     contest->GetName (),
                                     contest->GetName ());

      if (utf8)
      {
        iso8858 = g_convert (utf8,
                             -1,
                             "ISO-8859-1",
                             "UTF-8",
                             nullptr,
                             &bytes_written,
                             nullptr);

        if (iso8858)
        {
          fprintf (file, "%s",
                   iso8858);
          g_free (iso8858);
        }

        g_free (utf8);
      }
    }

    _fff_place_shifting = place_shifting;

    while (current_player)
    {
      Player    *player;
      Attribute *status_attr;

      player      = (Player *) current_player->data;
      status_attr = player->GetAttribute (&attr_id);

      if ((status_attr == nullptr) || (* (status_attr->GetStrValue ()) != 'E'))
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
      current_player = g_list_next (current_player);
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
    GError *error = nullptr;
    gchar  *result;

    {
      gsize  bytes_written;
      gchar *xml_image = attr->GetXmlImage ();

      if (strcmp (attr_name, "final_rank") == 0)
      {
        guint place = attr->GetIntValue ();

        g_free (xml_image);
        xml_image = g_strdup_printf ("%d", place + _fff_place_shifting);
      }
      else if (strcmp (attr_name, "birth_date") == 0)
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
                          nullptr,
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
