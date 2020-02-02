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
#include "util/xml_scheme.hpp"
#include "network/message.hpp"
#include "../../match.hpp"

#include "table_set.hpp"
#include "table_zone.hpp"

#include "table.hpp"

namespace Table
{
  // --------------------------------------------------------------------------------
  Table::Table (TableSet *table_set,
                guint     first_place,
                guint     size,
                guint     number,
                ...)
    : Object ("Table")
  {
    _table_set          = table_set;
    _size               = size;
    _number             = number;
    _column             = number;
    _left_table         = nullptr;
    _right_table        = nullptr;
    _first_error        = nullptr;
    _is_over            = FALSE;
    _ready_to_fence     = FALSE;
    _status_item        = nullptr;
    _header_item        = nullptr;
    _defeated_table_set = nullptr;
    _is_displayed       = TRUE;
    _loaded             = FALSE;
    _node_table         = g_new (GNode *, _size);
    _free_node_index    = 0;
    _mini_name          = g_strdup_printf ("T%d", _size);

    _match_list = nullptr;

    Disclose ("BellePoule::Batch");

    _job_list = new Net::Message ("BellePoule::JobList");
    _job_list->Set ("channel", (guint) Net::Ring::Channel::WEB_SOCKET);
    _job_list->Set ("client",  0);

    {
      va_list ap;

      va_start (ap, number);
      while (gchar *type = va_arg (ap, gchar *))
      {
        gchar *key = va_arg (ap, gchar *);

        if (type[0] == 'I')
        {
          guint value = va_arg (ap, guint);

          _parcel->Set   (key, value);
          _job_list->Set (key, value);

          if (g_strcmp0 (key, "stage") == 0)
          {
            _job_list->SetNetID (value);
          }
        }
        else if (type[0] == 'S')
        {
          gchar *value = va_arg (ap, gchar *);

          _parcel->Set   (key, value);
          _job_list->Set (key, value);
        }
      }
      va_end (ap);

      {
        gchar *name  = g_strdup_printf ("%d:%s",
                                        first_place,
                                        _mini_name);

        _parcel->Set   ("name", name);
        _job_list->Set ("name", name);

        g_free (name);
      }
    }
  }

  // --------------------------------------------------------------------------------
  Table::~Table ()
  {
    g_slist_free (_match_list);
    g_free (_node_table);
    g_free (_mini_name);
  }

  // --------------------------------------------------------------------------------
  void Table::ConfigureExtensions ()
  {
    if (_size >= 4)
    {
      gchar *extension = g_strdup_printf ("T%d", _size/2);

      _parcel->Set ("extension1", extension);
      g_free (extension);
    }
    if (_size >= 8)
    {
      gchar *extension = g_strdup_printf ("T%d", _size/4);

      _parcel->Set ("extension2", extension);
      g_free (extension);
    }
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
    if (b == nullptr)
    {
      return 1;
    }
    else if (a == nullptr)
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

    if (loosers)      *loosers      = nullptr;
    if (withdrawals)  *withdrawals  = nullptr;
    if (blackcardeds) *blackcardeds = nullptr;

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
                                       nullptr);
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
      GSList *simplified_list = nullptr;

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
            if (odd && (even->data == nullptr))
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
        GSList *ultimate_fencer = nullptr;
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

            if (current->data == nullptr)
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
                                                     nullptr);
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

    return nullptr;
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
    if (index < _size)
    {
      return _node_table[index];
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  gboolean Table::NodeHasGooTable (GNode *node)
  {
    NodeData *data = (NodeData *) node->data;

    return (data->_fencer_goo_table != nullptr);
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
    for (xmlNode *n = xml_node; n != nullptr; n = n->next)
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
                                                   nullptr,
                                                   16));
              xmlFree (attr);
            }
          }
        }
        else if (g_strcmp0 ((char *) n->name, "Match") == 0)
        {
          gchar *number = nullptr;

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
            guint   netid   = 0;

            {
              gchar *attr = (gchar *) xmlGetProp (n, BAD_CAST "NetID");

              if (attr)
              {
                netid = g_ascii_strtoull (attr,
                                          nullptr,
                                          16);
                xmlFree (attr);
              }
            }

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
                  Stage *stage = _table_set->GetStage ();

                  match->ChangeIdChain (_parcel->GetNetID (),
                                        netid);

                  stage->LoadMatch (n,
                                    match);

                  for (guint i = 0; i < 2; i++)
                  {
                    _table_set->SetPlayerToMatch (match,
                                                  match->GetOpponent (i),
                                                  i);
                  }
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
  void Table::SaveHeader (XmlScheme *xml_scheme)
  {
    xml_scheme->StartElement ("Tableau");
    xml_scheme->WriteFormatAttribute ("ID",
                                      "A%d", _size);

    xml_scheme->WriteFormatAttribute ("NetID",
                                      "%x", _parcel->GetNetID ());

    {
      gchar *image = GetImage ();

      xml_scheme->WriteAttribute ("Titre",
                                  image);
      g_free (image);
    }

    xml_scheme->WriteFormatAttribute ("Taille",
                                      "%d", _size);

    if (_defeated_table_set)
    {
      xml_scheme->WriteAttribute ("DestinationDesElimines",
                                  _defeated_table_set->GetId ());
    }
  }

  // --------------------------------------------------------------------------------
  void Table::Save (XmlScheme *xml_scheme)
  {
    SaveHeader (xml_scheme);

    {
      GSList *current_match = _match_list;

      while (current_match)
      {
        Match *match = (Match *) current_match->data;

        match->Save (xml_scheme);

        current_match = g_slist_next (current_match);
      }
    }

    xml_scheme->EndElement ();
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
  gboolean Table::IsHeaderDisplayed ()
  {
    return _is_header_displayed;
  }

  // --------------------------------------------------------------------------------
  void Table::Show (guint    at_column,
                    gboolean display_header)
  {
    _is_displayed        = TRUE;
    _is_header_displayed = display_header;
    _column              = at_column;
  }

  // --------------------------------------------------------------------------------
  void Table::Hide ()
  {
    _is_displayed        = FALSE;
    _is_header_displayed = FALSE;
  }

  // --------------------------------------------------------------------------------
  void Table::FeedParcel (Net::Message *parcel)
  {
    parcel->Set ("done", _is_over);
  }

  // --------------------------------------------------------------------------------
  void Table::Spread ()
  {
    gboolean dirty       = FALSE;
    guint    match_count = 0;

    if (_parcel->GetInteger ("done") != (guint) (_is_over))
    {
      dirty = TRUE;
    }

    for (GSList *current = _match_list; current; current = g_slist_next (current))
    {
      Match *match = (Match *) current->data;

      if (match->GetOpponent (0) && match->GetOpponent (1))
      {
        if (match->IsDirty ())
        {
          dirty = TRUE;
        }
        match_count++;
      }
    }

    if (_left_table)
    {
      _parcel->Set ("ready_jobs",    match_count);
      _parcel->Set ("expected_jobs", _size/2);
    }
    else
    {
      _parcel->Set ("ready_jobs",    match_count);
      _parcel->Set ("expected_jobs", match_count);
    }

    if (dirty)
    {
      // BellePoule::JobList
      if (_ready_to_fence)
      {
        xmlBuffer *xml_buffer = xmlBufferCreate ();

        {
          XmlScheme *xml_scheme = new XmlScheme (xml_buffer);

          _table_set->SaveHeaders (xml_scheme);
          SaveHeader (xml_scheme);

          for (GSList *m = _match_list; m; m = g_slist_next (m))
          {
            Match *match = (Match *) m->data;

            if (match->GetOpponent (0) && match->GetOpponent (1) && match->IsDirty ())
            {
              match->Save (xml_scheme);
            }
          }

          xml_scheme->EndElement ();
          xml_scheme->EndElement ();

          xml_scheme->Release ();
        }

        _job_list->Set ("xml", (const gchar *) xml_buffer->content);

        xmlBufferFree (xml_buffer);

        _job_list->Spread ();
      }

      // BellePoule::Batch
      {
        Object::Spread ();

        for (GSList *current = _match_list; current; current = g_slist_next (current))
        {
          Match *match = (Match *) current->data;

          if (match->GetOpponent (0) && match->GetOpponent (1) && match->IsDirty ())
          {
            match->Spread ();
          }
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Table::Recall ()
  {
    _job_list->Recall ();

    {
      GSList *current = _match_list;

      while (current)
      {
        Match *match = (Match *) current->data;

        match->Recall ();

        current = g_slist_next (current);
      }
    }

    Object::Recall ();
  }

  // --------------------------------------------------------------------------------
  void Table::ClearRoadmaps ()
  {
    GSList *current_match = _match_list;

    while (current_match)
    {
      Match *match = (Match *) current_match->data;

      match->SetPiste          (0);
      match->SetStartTime      (nullptr);
      match->SetDurationSpan   (1);
      match->RemoveAllReferees ();

      current_match = g_slist_next (current_match);
    }
  }

  // --------------------------------------------------------------------------------
  const gchar *Table::GetMiniName ()
  {
    return _mini_name;
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

  // --------------------------------------------------------------------------------
  Error *Table::SpawnError ()
  {
    gchar *guilty_party = GetImage ();
    Error *error;

    error = new Error (Error::Level::WARNING,
                       guilty_party,
                       gettext ("Referees allocation \n ongoing"));

    g_free (guilty_party);

    return error;
  }
}
