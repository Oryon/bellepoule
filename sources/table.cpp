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

#include "attribute.hpp"
#include "player.hpp"
#include "table_set.hpp"

#include "table.hpp"

// --------------------------------------------------------------------------------
Table::Table (TableSet *table_set,
              guint     size,
              guint     number)
  : Object ("table")
{
  _table_set          = table_set;
  _size               = size;
  _number             = number;
  _column             = number;
  _left_table         = NULL;
  _right_table        = NULL;
  _has_error          = FALSE;
  _is_over            = FALSE;
  _status_item        = NULL;
  _header_item        = NULL;
  _defeated_table_set = NULL;
  _is_displayed       = TRUE;
  _loaded             = FALSE;

  _match_list = NULL;
}

// --------------------------------------------------------------------------------
Table::~Table ()
{
  g_slist_free (_match_list);
}

// --------------------------------------------------------------------------------
guint Table::GetSize ()
{
  return _size;
}

// --------------------------------------------------------------------------------
void Table::SetRightTable (Table *right)
{
  _right_table = right;
  if (_right_table)
  {
    _right_table->_left_table = this;
  }
}

// --------------------------------------------------------------------------------
Table *Table::GetRightTable ()
{
  return _right_table;
}

// --------------------------------------------------------------------------------
Table *Table::GetLeftTable ()
{
  return _left_table;
}

// --------------------------------------------------------------------------------
gint Table::CompareMatchNumber (Match *a,
                                Match *b)
{
  if (b == NULL)
  {
    return 1;
  }
  else if (a == NULL)
  {
    return -1;
  }
  else
  {
    return a->GetNumber () - b->GetNumber ();
  }
}

// --------------------------------------------------------------------------------
guint Table::GetLoosers (GSList **loosers,
                         GSList **withdrawals,
                         GSList **blackcardeds)
{
  GSList   *current_match = _match_list;
  gboolean  nb_loosers    = 0;

  if (loosers)      *loosers      = NULL;
  if (withdrawals)  *withdrawals  = NULL;
  if (blackcardeds) *blackcardeds = NULL;

  while (current_match)
  {
    Match *match = (Match *) current_match->data;

    if (match)
    {
      Player *looser = match->GetLooser ();

      if (looser && match->IsDropped ())
      {
        Player::AttributeId  attr_id ("status", _table_set->GetDataOwner ());
        Attribute           *attr   = looser->GetAttribute (&attr_id);
        gchar               *status = attr->GetStrValue ();

        if (status)
        {
          if (withdrawals && (*status == 'A'))
          {
            *withdrawals = g_slist_append (*withdrawals,
                                           looser);
          }
          if (blackcardeds && (*status == 'E'))
          {
            *blackcardeds = g_slist_append (*blackcardeds,
                                            looser);
          }
        }

        if (loosers)
        {
          *loosers = g_slist_append (*loosers,
                                     NULL);
        }
      }
      else
      {
        if (looser)
        {
          nb_loosers++;
        }
        if (loosers)
        {
          *loosers = g_slist_append (*loosers,
                                     looser);
        }
      }
    }
    current_match = g_slist_next (current_match);
  }

  return nb_loosers;
}

// --------------------------------------------------------------------------------
void Table::ManageMatch (Match *match)
{
  _match_list = g_slist_insert_sorted (_match_list,
                                       match,
                                       (GCompareFunc) CompareMatchNumber);
}

// --------------------------------------------------------------------------------
void Table::DropMatch (Match *match)
{
  _match_list = g_slist_remove (_match_list,
                                match);
}

// --------------------------------------------------------------------------------
void Table::Load (xmlNode *xml_node)
{
  for (xmlNode *n = xml_node; n != NULL; n = n->next)
  {
    if (n->type == XML_ELEMENT_NODE)
    {
      if ((_loaded == FALSE) && (strcmp ((char *) n->name, "Tableau") == 0))
      {
        _loaded = TRUE;
      }
      else if (strcmp ((char *) n->name, "Match") == 0)
      {
        gchar  *attr    = (gchar *) xmlGetProp (n, BAD_CAST "ID");
        GSList *current = _match_list;
        gchar  *number  = g_strdup_printf ("%d.%s", GetSize (), attr);

        while (current)
        {
          Match *match = (Match *) current->data;
          gchar *id = match->GetName ();

          id = strstr (id, "-");
          if (id)
          {
            id++;
            if (strcmp (id, number) == 0)
            {
              LoadMatch (n,
                         match);
              break;
            }
          }
          current = g_slist_next (current);
        }
        g_free (number);
      }
      else
      {
        return;
      }
    }
    Load (n->children);
  }
}

// --------------------------------------------------------------------------------
void Table::LoadMatch (xmlNode *xml_node,
                       Match   *match)
{
  for (xmlNode *n = xml_node; n != NULL; n = n->next)
  {
    if (n->type == XML_ELEMENT_NODE)
    {
      static xmlNode *A = NULL;
      static xmlNode *B = NULL;

      if (strcmp ((char *) n->name, "Match") == 0)
      {
        gchar *attr = (gchar *) xmlGetProp (n, BAD_CAST "ID");

        A = NULL;
        B = NULL;
        if ((attr == NULL) || (atoi (attr) != match->GetNumber ()))
        {
          return;
        }
      }
      else if (strcmp ((char *) n->name, "Arbitre") == 0)
      {
      }
      else if (strcmp ((char *) n->name, "Tireur") == 0)
      {
        if (A == NULL)
        {
          A = n;
        }
        else
        {
          Player *dropped = NULL;

          B = n;

          LoadScore (A, match, 0, &dropped);
          LoadScore (B, match, 1, &dropped);

          if (dropped)
          {
            match->DropPlayer (dropped);
          }

          A = NULL;
          B = NULL;
          return;
        }
      }
      LoadMatch (n->children,
                 match);
    }
  }
}

// --------------------------------------------------------------------------------
void Table::LoadScore (xmlNode *xml_node,
                       Match   *match,
                       guint    player_index,
                       Player  **dropped)
{
  gboolean  is_the_best = FALSE;
  gchar    *attr        = (gchar *) xmlGetProp (xml_node, BAD_CAST "REF");
  Player   *player      = _table_set->GetFencerFromRef (atoi (attr));

  _table_set->SetPlayerToMatch (match,
                                player,
                                player_index);

  attr = (gchar *) xmlGetProp (xml_node, BAD_CAST "Statut");
  if (attr && attr[0] == 'V')
  {
    is_the_best = TRUE;
  }

  attr = (gchar *) xmlGetProp (xml_node, BAD_CAST "Score");
  if (attr)
  {
    match->SetScore (player, atoi (attr), is_the_best);

    if (is_the_best == FALSE)
    {
      Player::AttributeId  attr_id ("status", _table_set->GetDataOwner ());
      Attribute           *attr   = player->GetAttribute (&attr_id);
      gchar               *status = attr->GetStrValue ();

      if (status && ((*status == 'E') || (*status == 'A')))
      {
        *dropped = player;
      }
    }
  }
}

// --------------------------------------------------------------------------------
void Table::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST "Tableau");
  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "ID",
                                     "A%d", _size);
  xmlTextWriterWriteAttribute (xml_writer,
                               BAD_CAST "Titre",
                               BAD_CAST GetImage ());
  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "Taille",
                                     "%d", _size);
  if (_defeated_table_set)
  {
    xmlTextWriterWriteAttribute (xml_writer,
                                 BAD_CAST "DestinationDesElimines",
                                 BAD_CAST _defeated_table_set->GetId ());
  }

  {
    GSList *current_match = _match_list;

    while (current_match)
    {
      Match *match = (Match *) current_match->data;

      match->Save (xml_writer);

      current_match = g_slist_next (current_match);
    }
  }

  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
guint Table::GetRow (guint for_index)
{
  guint first_index_row = (1 << _column) - 1;
  guint delta_row       = (2 << _column);

  return first_index_row + for_index*delta_row;
}

// --------------------------------------------------------------------------------
guint Table::GetNumber ()
{
  return _number;
}

// --------------------------------------------------------------------------------
guint Table::GetColumn ()
{
  return _column;
}

// --------------------------------------------------------------------------------
gboolean Table::IsDisplayed ()
{
  return _is_displayed;
}

// --------------------------------------------------------------------------------
void Table::Show (guint at_column)
{
  _is_displayed = TRUE;
  _column       = at_column;
}

// --------------------------------------------------------------------------------
void Table::Hide ()
{
  _is_displayed = FALSE;
}

// --------------------------------------------------------------------------------
gchar *Table::GetImage ()
{
  gchar *image;

  if (_size == 1)
  {
    image = g_strdup_printf (gettext ("Winner"));
  }
  else if (_size == 2)
  {
    image = g_strdup_printf (gettext ("Final"));
  }
  else if (_size == 4)
  {
    image = g_strdup_printf (gettext ("Semi-final"));
  }
  else
  {
    image = g_strdup_printf (gettext ("Table of %d"), _size);
  }

  return image;
}
