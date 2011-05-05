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

#ifndef table_hpp
#define table_hpp

#include <gtk/gtk.h>
#include <goocanvas.h>
#include <libxml/xmlwriter.h>

#include "object.hpp"
#include "match.hpp"

class TableSet;

class Table : public Object
{
  public:
    Table (TableSet *table_set,
           guint     size,
           guint     column);

    gchar *GetImage ();

    void SetRightTable (Table *right);

    Table *GetRightTable ();

    Table *GetLeftTable ();

    guint GetSize ();

    guint GetRow (guint for_index);

    guint GetColumn ();

    void Show (guint at_column);

    void Hide ();

    void Save (xmlTextWriter *xml_writer);

    void Load (xmlNode *xml_node);

    gboolean IsDisplayed ();

    void ManageMatch (Match *match);

    void DropMatch (Match *match);

    GSList *GetLoosers ();

    gboolean       _has_error;
    guint          _is_over;
    GooCanvasItem *_status_item;
    GooCanvasItem *_header_item;
    gchar         *_defeated_table_set;

  private:
    guint     _size;
    guint     _column;
    gboolean  _is_displayed;
    Table    *_left_table;
    Table    *_right_table;
    GSList   *_match_list;
    TableSet *_table_set;

    ~Table ();

    static gint CompareMatchNumber (Match *a,
    		                        Match *b);

    void LoadMatch (xmlNode *xml_node,
                    Match   *match);

    void LoadScore (xmlNode *xml_node,
                    Match   *match,
                    guint    player_index,
                    Player  **dropped);
};

#endif
