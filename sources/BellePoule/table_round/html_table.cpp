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

#include "html_table.hpp"

#define CONNECTOR ((void *) -1)

namespace Table
{
  // --------------------------------------------------------------------------------
  HtmlTable::HtmlTable (Object *owner)
    : Object ("HtmlTable")
  {
    _owner        = owner;
    _table        = NULL;
    _table_width  = 0;
    _table_height = 0;
  }

  // --------------------------------------------------------------------------------
  HtmlTable::~HtmlTable ()
  {
    g_free (_table);
  }

  // --------------------------------------------------------------------------------
  void HtmlTable::Prepare (guint column_count)
  {
    _table_width  = column_count;
    _table_height = (1 << _table_width) - 1;

    Clean ();
  }

  // --------------------------------------------------------------------------------
  void HtmlTable::Clean ()
  {
    g_free (_table);
    _table = g_new0 (void *, _table_height * _table_width);
  }

  // --------------------------------------------------------------------------------
  void HtmlTable::Put (Match *match,
                       guint  row,
                       guint  column)
  {
    _table[row*_table_width + column] = match;

    match->SetData (NULL, "HtmlTable::row", (void *) row);
    match->SetData (NULL, "HtmlTable::col", (void *) column);
  }

  // --------------------------------------------------------------------------------
  void HtmlTable::Connect (Match *m1,
                           Match *m2)
  {
    guint row1 = m1->GetUIntData (NULL, "HtmlTable::row");
    guint row2 = m2->GetUIntData (NULL, "HtmlTable::row");
    guint col2 = m2->GetUIntData (NULL, "HtmlTable::col");

    if (row1 > row2)
    {
      guint swap = row1;

      row1 = row2;
      row2 = swap;
    }

    for (guint r = row1+1; r < row2; r++)
    {
      _table[r*_table_width + col2] = (void *) CONNECTOR;
    }

  }

  // --------------------------------------------------------------------------------
  void HtmlTable::DumpToHTML (FILE                *file,
                              Player              *fencer,
                              const gchar         *attr_name,
                              AttributeDesc::Look  look)

  {
    AttributeDesc       *desc = AttributeDesc::GetDescFromCodeName (attr_name);
    Player::AttributeId *attr_id;
    Attribute           *attr;

    if (desc->_scope == AttributeDesc::LOCAL)
    {
      attr_id = new Player::AttributeId (desc->_code_name,
                                         _owner);
    }
    else
    {
      attr_id = new Player::AttributeId (desc->_code_name);
    }

    attr = fencer->GetAttribute (attr_id);
    attr_id->Release ();

    if (attr)
    {
      gchar *attr_image = attr->GetUserImage (look);

      fprintf (file, "<span class=\"%s\">%s </span>", attr_name, attr_image);
      g_free (attr_image);
    }
  }

  // --------------------------------------------------------------------------------
  void HtmlTable::DumpToHTML (FILE *file)
  {
    for (guint r = 0; r < _table_height; r++)
    {
      fprintf (file, "            <tr>\n");
      for (guint c = 0; c < _table_width; c++)
      {
        void *data = _table[r*_table_width + c];

        if (data == CONNECTOR)
        {
          fprintf (file, "              <td class=\"TableConnector\">|</td>\n");
        }
        else if (data)
        {
          Match  *match  = (Match *) data;
          Player *winner = (Player *) match->GetWinner ();

          if (c == 0)
          {
            fprintf (file, "              <td class=\"TableCellFirstCol\">");
          }
          else if (c == _table_width-1)
          {
            fprintf (file, "              <td class=\"TableCellLastCol\">");
          }
          else
          {
            fprintf (file, "              <td class=\"TableCell\">");
          }

          if (match->GetName ())
          {
            Score *scoreA = match->GetScore ((guint) 0);
            Score *scoreB = match->GetScore ((guint) 1);

            fprintf (file, "<span class=\"TableScore\">%s-%s</span>", scoreA->GetImage (), scoreB->GetImage ());
          }

          if (winner)
          {
            DumpToHTML (file,
                        winner,
                        "name",
                        AttributeDesc::LONG_TEXT);
            DumpToHTML (file,
                        winner,
                        "first_name",
                        AttributeDesc::LONG_TEXT);
            DumpToHTML (file,
                        winner,
                        "country",
                        AttributeDesc::SHORT_TEXT);
          }

          fprintf (file, "</td>\n");
        }
        else
        {
          fprintf (file, "              <td class=\"EmptyTableCell\"></td>\n");
        }
      }
      fprintf (file, "            </tr>\n");
    }
  }
}
