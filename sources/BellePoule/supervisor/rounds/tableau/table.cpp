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

#include "util/attribute.hpp"
#include "util/player.hpp"
#include "util/fie_time.hpp"
#include "network/message.hpp"

#include "table_set.hpp"
#include "table_zone.hpp"

#include "table.hpp"

namespace Table
{
  // --------------------------------------------------------------------------------
  Table::Table (TableSet    *table_set,
                const gchar *xml_player_tag,
                guint        size,
                guint        number,
                ...)
    : Object ("Table")
  {
    _table_set          = table_set;
    _size               = size;
    _number             = number;
    _column             = number;
    _left_table         = NULL;
    _right_table        = NULL;
    _first_error        = NULL;
    _is_over            = FALSE;
    _ready_to_fence     = FALSE;
    _has_all_roadmap    = FALSE;
    _roadmap_count      = 0;
    _status_item        = NULL;
    _header_item        = NULL;
    _defeated_table_set = NULL;
    _is_displayed       = TRUE;
    _loaded             = FALSE;
    _node_table         = g_new (GNode *, _size);
    _free_node_index    = 0;
    _xml_player_tag     = xml_player_tag;

    _match_list = NULL;

    Disclose ("Batch");

    {
      va_list ap;

      va_start (ap, number);
      while (gchar *key = va_arg (ap, gchar *))
      {
        guint value = va_arg (ap, guint);

        _parcel->Set (key, value);
      }
      va_end (ap);

      {
        gchar *image = g_strdup_printf ("%s\n%s",
                                        _table_set->GetName (),
                                        GetImage ());

        _parcel->Set ("name", image);

        g_free (image);
      }
    }
  }

  // --------------------------------------------------------------------------------
  Table::~Table ()
  {
    g_slist_free (_match_list);
    g_free (_node_table);
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

    SimplifyLooserTree (loosers);

    return nb_loosers;
  }

  // --------------------------------------------------------------------------------
  void Table::SimplifyLooserTree (GSList **list)
  {
    if (list)
    {
      GSList *simplified_list = NULL;

      // Remove empty branches from the top of the tree
      {
        GSList *current = *list;

        while (current)
        {
          GSList *even;
          GSList *odd;

          even    = current;
          current = g_slist_next (current);
          odd     = current;
          if (even->data || (odd && odd->data))
          {
            simplified_list = g_slist_copy (even);

            // If applicable, invert the NULL player (withdrawal) with its opponent
            // to make looser trees nicer.
            if (odd && (even->data == NULL))
            {
              simplified_list = g_slist_remove (simplified_list,
                                                odd->data);
              simplified_list = g_slist_prepend (simplified_list,
                                                 odd->data);
            }
            break;
          }
          current = g_slist_next (current);
        }
      }

      // Remove empty branches from the bottom of the tree
      {
        GSList *ultimate_fencer = NULL;
        GSList *current         = simplified_list;

        while (current)
        {
          if (current->data)
          {
            ultimate_fencer = current;
          }

          current = g_slist_next (current);
        }

        if (ultimate_fencer)
        {
          current = g_slist_next (ultimate_fencer);
          while (current)
          {
            GSList *next = g_slist_next (current);

            if (current->data == NULL)
            {
              simplified_list = g_slist_delete_link (simplified_list,
                                                     current);
            }

            current = next;
          }

          if (g_slist_length (simplified_list) % 2 != 0)
          {
            simplified_list = g_slist_insert_before (simplified_list,
                                                     ultimate_fencer,
                                                     NULL);
          }
        }
      }

      g_slist_free (*list);
      *list = simplified_list;
    }
  }

  // --------------------------------------------------------------------------------
  void Table::ManageMatch (Match *match,
                           ...)
  {
    _match_list = g_slist_insert_sorted (_match_list,
                                         match,
                                         (GCompareFunc) CompareMatchNumber);

    {
      va_list ap;

      va_start (ap, match);
      match->DiscloseWithIdChain (ap);
      va_end (ap);
    }
  }

  // --------------------------------------------------------------------------------
  Match *Table::GetMatch (Net::Message *roadmap)
  {
    if (_parcel->GetNetID () == roadmap->GetInteger ("batch"))
    {
      guint   source        = roadmap->GetInteger ("source");
      GSList *current_match = _match_list;

      while (current_match)
      {
        Match *match = (Match *) current_match->data;

        if (source == match->GetNetID ())
        {
          return match;
        }

        current_match = g_slist_next (current_match);
      }
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  void Table::AddNode (GNode *node)
  {
    if (_free_node_index < _size)
    {
      _node_table[_free_node_index] = node;
      _free_node_index++;
    }
  }

  // --------------------------------------------------------------------------------
  GNode *Table::GetNode (guint index)
  {
    return _node_table[index];
  }

  // --------------------------------------------------------------------------------
  gboolean Table::NodeHasGooTable (GNode *node)
  {
    NodeData *data = (NodeData *) node->data;

    return (data->_fencer_goo_table != NULL);
  }

  // --------------------------------------------------------------------------------
  void Table::DropMatch (Match *match)
  {
    _match_list = g_slist_remove (_match_list,
                                  match);
    match->Recall ();
  }

  // --------------------------------------------------------------------------------
  void Table::Load (xmlNode *xml_node)
  {
    for (xmlNode *n = xml_node; n != NULL; n = n->next)
    {
      if (n->type == XML_ELEMENT_NODE)
      {
        if ((_loaded == FALSE) && (g_strcmp0 ((char *) n->name, "Tableau") == 0))
        {
          _loaded = TRUE;

          {
            gchar *attr = (gchar *) xmlGetProp (xml_node, BAD_CAST "NetID");

            if (attr)
            {
              _parcel->SetNetID (g_ascii_strtoull (attr,
                                                   NULL,
                                                   16));
              xmlFree (attr);
            }
          }
        }
        else if (g_strcmp0 ((char *) n->name, "Match") == 0)
        {
          gchar *number = NULL;

          {
            gchar *attr = (gchar *) xmlGetProp (n, BAD_CAST "ID");

            if (attr)
            {
              number = g_strdup_printf ("%d.%s", GetSize (), attr);

              xmlFree (attr);
            }
          }

          if (number)
          {
            GSList *current = _match_list;

            while (current)
            {
              Match       *match = (Match *) current->data;
              const gchar *id    = match->GetName ();

              id = strstr (id, "-");
              if (id)
              {
                id++;
                if (g_strcmp0 (id, number) == 0)
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

        if (g_strcmp0 ((char *) n->name, "Match") == 0)
        {
          {
            gchar *attr = (gchar *) xmlGetProp (n, BAD_CAST "ID");

            A = NULL;
            B = NULL;
            if ((attr == NULL) || (atoi (attr) != match->GetNumber ()))
            {
              xmlFree (attr);
              return;
            }
            xmlFree (attr);
          }

          {
            gchar *attr = (gchar *) xmlGetProp (xml_node, BAD_CAST "Piste");

            if (attr)
            {
              match->SetPiste (atoi (attr));
              xmlFree (attr);
            }
          }

          {
            gchar *attr = (gchar *) xmlGetProp (xml_node, BAD_CAST "Date");

            if (attr)
            {
              match->SetStartTime (new FieTime (attr));

              xmlFree (attr);
            }
          }
        }
        else if (g_strcmp0 ((char *) n->name, "Arbitre") == 0)
        {
          gchar *attr = (gchar *) xmlGetProp (n, BAD_CAST "REF");

          if (attr)
          {
            _table_set->AddReferee (match,
                                    atoi (attr));
            xmlFree (attr);
          }
        }
        else if (g_strcmp0 ((char *) n->name, _xml_player_tag) == 0)
        {
          if (A == NULL)
          {
            A = n;
          }
          else
          {
            B = n;

            {
              Player *fencer_a = NULL;
              Player *fencer_b = NULL;
              gchar  *attr;

              attr = (gchar *) xmlGetProp (A, BAD_CAST "REF");
              if (attr)
              {
                fencer_a = _table_set->GetFencerFromRef (atoi (attr));
                xmlFree (attr);
              }

              attr = (gchar *) xmlGetProp (B, BAD_CAST "REF");
              if (attr)
              {
                fencer_b = _table_set->GetFencerFromRef (atoi (attr));
                xmlFree (attr);
              }

              if (fencer_a && fencer_b)
              {
                match->Load (A, fencer_a,
                             B, fencer_b);

                _table_set->SetPlayerToMatch (match,
                                              fencer_a,
                                              0);
                _table_set->SetPlayerToMatch (match,
                                              fencer_b,
                                              1);
              }
            }

            if (match->IsOver ())
            {
              TableZone *zone = (TableZone *) match->GetPtrData (_table_set,
                                                                 "drop_zone");
              zone->FreeReferees ();
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
  void Table::SaveHeader (xmlTextWriter *xml_writer)
  {
    xmlTextWriterStartElement (xml_writer,
                               BAD_CAST "Tableau");
    xmlTextWriterWriteFormatAttribute (xml_writer,
                                       BAD_CAST "ID",
                                       "A%d", _size);

    xmlTextWriterWriteFormatAttribute (xml_writer,
                                       BAD_CAST "NetID",
                                       "%x", _parcel->GetNetID ());

    {
      gchar *image = GetImage ();

      xmlTextWriterWriteAttribute (xml_writer,
                                   BAD_CAST "Titre",
                                   BAD_CAST image);
      g_free (image);
    }

    xmlTextWriterWriteFormatAttribute (xml_writer,
                                       BAD_CAST "Taille",
                                       "%d", _size);

    if (_defeated_table_set)
    {
      xmlTextWriterWriteAttribute (xml_writer,
                                   BAD_CAST "DestinationDesElimines",
                                   BAD_CAST _defeated_table_set->GetId ());
    }
  }

  // --------------------------------------------------------------------------------
  void Table::Save (xmlTextWriter *xml_writer)
  {
    SaveHeader (xml_writer);

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
  void Table::FeedParcel (Net::Message *parcel)
  {
    parcel->Set ("done", _is_over);
  }

  // --------------------------------------------------------------------------------
  void Table::Spread ()
  {
    if (_parcel->IsSpread () == FALSE)
    {
      Object::Spread ();

      {
        GSList *current = _match_list;

        while (current)
        {
          Match *match = (Match *) current->data;

          if (match->GetOpponent (0) && match->GetOpponent (1))
          {
            match->Spread ();
          }

          current = g_slist_next (current);
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Table::Recall ()
  {
    Object::Recall ();

    {
      GSList *current = _match_list;

      while (current)
      {
        Match *match = (Match *) current->data;

        match->Recall ();

        current = g_slist_next (current);
      }
    }
  }

  // --------------------------------------------------------------------------------
  gchar *Table::GetImage ()
  {
    gchar *image;
    gchar *printable_image;

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

    printable_image = GetUndivadableText (image);
    g_free (image);

    return printable_image;
  }

  // --------------------------------------------------------------------------------
  gchar *Table::GetAnnounce ()
  {
    gchar *image    = GetImage ();
    gchar *announce = g_strdup_printf ("%s %s", image, gettext ("OVER"));

    g_free (image);
    return announce;
  }
}
